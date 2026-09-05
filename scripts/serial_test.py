# /// script
# requires-python = ">=3.11"
# dependencies = [
#     "pyserial>=3.5",
# ]
# ///
"""Step 36: send +100/+100 RPM at 10 Hz for five seconds, then request stop."""

import argparse
import time

import serial


class TelemetryReader:
    """Keep incomplete lines across serial read timeouts, with bounded storage."""

    def __init__(self, port):
        self.port = port
        self.pending = bytearray()
        self.discarding = False

    def readline(self):
        chunk = self.port.readline()
        if self.discarding:
            if chunk.endswith(b"\n"):
                self.discarding = False
            return b""
        self.pending.extend(chunk)
        if len(self.pending) > 96:
            self.pending.clear()
            self.discarding = not chunk.endswith(b"\n")
            return b""
        if not self.pending.endswith(b"\n"):
            return b""
        line = bytes(self.pending)
        self.pending.clear()
        return line


def read_sample(port):
    """Return one complete six-field telemetry sample, or None."""
    raw = port.readline()
    if not raw.endswith(b"\n"):
        return None
    fields = raw.strip().split(b",")
    if len(fields) != 7 or fields[0] != b"S":
        return None
    try:
        sample = tuple(int(value) for value in fields[1:])
    except ValueError:
        return None
    print(f"{time.monotonic():.3f} RX {raw.decode('ascii').strip()}", flush=True)
    return sample


def send(port, command):
    if port.write(command) != len(command):
        raise RuntimeError("Incomplete serial write")
    print(f"{time.monotonic():.3f} TX {command.decode('ascii').strip()}", flush=True)


def observe_watchdog(port, last_command, reader=None):
    """Observe command silence; host timestamps are not MCU timing measurements."""
    print("WATCHDOG: TX paused; waiting for firmware timeout (no STOP sent).")
    active_seen = False
    cutoff_time = None
    stopped_samples = 0
    if reader is None:
        reader = TelemetryReader(port)
    while time.monotonic() - last_command < 1.5:
        sample = read_sample(reader)
        elapsed = time.monotonic() - last_command
        if sample is not None:
            disabled = all(sample[index] == 0 for index in (0, 2, 3, 5))
            if cutoff_time is None:
                if sample[0] == sample[3] == 100 and sample[2] > 0 and sample[5] > 0:
                    active_seen = True
                if disabled:
                    if not active_seen or not 0.4 <= elapsed <= 0.7:
                        raise RuntimeError(
                            "Watchdog: missing active feedback or invalid cutoff timing"
                        )
                    cutoff_time = elapsed
            elif not disabled:
                raise RuntimeError(
                    "Watchdog: targets/output reactivated during command silence"
                )
            stopped_samples = stopped_samples + 1 if not any(sample) else 0
            if cutoff_time is not None and stopped_samples >= 3:
                print(
                    f"WATCHDOG PASS (telemetry): cutoff observed at {cutoff_time * 1000:.0f} ms; "
                    "three stopped samples received before cleanup STOP."
                )
                return
        if elapsed > 0.7 and cutoff_time is None:
            raise RuntimeError(
                "Watchdog: no target/output cutoff observed within 700 ms"
            )
    raise RuntimeError("Watchdog: complete stop not confirmed within 1.5 s")


def run_test(port, watchdog=False):
    reader = TelemetryReader(port)
    try:
        # Require live stopped telemetry before sending any nonzero command.
        port.reset_input_buffer()
        deadline = time.monotonic() + 3.0
        stopped_samples = 0
        while time.monotonic() < deadline and stopped_samples < 10:
            sample = read_sample(reader)
            if sample is None:
                continue
            if any(sample):
                raise RuntimeError(
                    "Preflight: motor is not stopped; check APP_TEST_MODE=0"
                )
            stopped_samples += 1
        if stopped_samples < 10:
            raise RuntimeError(
                "Preflight: missing stopped telemetry; no motion command sent"
            )

        start = time.monotonic()
        next_send = start
        last_command = start
        last_sample = start
        while time.monotonic() - start < 5.0:
            now = time.monotonic()
            if now - last_sample > 0.5:
                raise RuntimeError("Telemetry lost for over 500 ms")
            if now >= next_send:
                send(port, b"V,100,100\n")
                last_command = time.monotonic()
                next_send = now + 0.1
            sample = read_sample(reader)
            if sample is not None:
                last_sample = time.monotonic()
                if sample[1] < -10 or sample[4] < -10:
                    raise RuntimeError("Reverse feedback during positive speed command")
                if abs(sample[2]) >= 400 or abs(sample[5]) >= 400:
                    raise RuntimeError("Motor output reached the configured 400 limit")
        if watchdog:
            observe_watchdog(port, last_command, reader)
    finally:
        # Watchdog observation completes/fails before this cleanup command.
        send(port, b"V,0,0\n")

    if watchdog:
        return

    deadline = time.monotonic() + 2.0
    stopped_samples = 0
    while time.monotonic() < deadline:
        sample = read_sample(reader)
        if sample is not None:
            stopped_samples = stopped_samples + 1 if not any(sample) else 0
            if stopped_samples >= 3:
                print("Stopped telemetry observed. Review RPM tracking in the log.")
                return
    raise RuntimeError("Stop requested, but stopped telemetry was not confirmed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="CH340 serial port, for example COM5")
    parser.add_argument(
        "--watchdog",
        action="store_true",
        help="After five seconds, stop TX and observe firmware timeout before cleanup",
    )
    args = parser.parse_args()
    try:
        # Configure control lines before opening; do not deliberately reset the MCU.
        with serial.Serial(
            port=None, baudrate=115200, timeout=0.01, write_timeout=0.2
        ) as port:
            port.dtr = False
            port.rts = False
            port.port = args.port
            port.open()
            run_test(port, watchdog=args.watchdog)
        return 0
    except KeyboardInterrupt:
        print(
            "Interrupted; stop requested. Confirm motors stopped; otherwise cut motor power."
        )
        return 130
    except (serial.SerialException, RuntimeError) as error:
        print(f"ERROR: {error}. Confirm motors stopped; otherwise cut motor power.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
