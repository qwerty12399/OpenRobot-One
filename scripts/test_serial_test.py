"""Host-only tests; never open a serial device or send hardware commands."""

import contextlib
import io
import unittest
from itertools import pairwise
from unittest.mock import patch

import serial_test


class FakePort:
    def __init__(self, failure=None):
        self.now = 0.0
        self.writes = []
        self.failure = failure
        self.moving = False

    def reset_input_buffer(self):
        pass

    def write(self, command):
        self.writes.append((self.now, command))
        self.moving = command == b"V,100,100\n"
        return len(command)

    def readline(self):
        self.now += 0.01
        if self.failure == "no_telemetry":
            return b""
        if self.moving:
            if (
                self.failure == "working_watchdog"
                and self.now - self.writes[-1][0] > 0.5
            ):
                self.moving = False
                return b"S,0,0,0,0,0,0\r\n"
            if self.failure == "disconnect":
                return b""
            if self.failure == "interrupt":
                raise KeyboardInterrupt
            if self.failure == "reverse":
                return b"S,100,99,205,100,-100,220\r\n"
            return b"S,100,99,205,100,102,204\r\n"
        return b"S,0,0,0,0,0,0\r\n"


class SerialTestTests(unittest.TestCase):
    def run_fake(self, port, watchdog=False):
        with (
            patch.object(serial_test.time, "monotonic", lambda: port.now),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            serial_test.run_test(port, watchdog=watchdog)

    def test_watchdog_stops_before_cleanup_command(self):
        port = FakePort("working_watchdog")
        self.run_fake(port, watchdog=True)
        self.assertEqual(port.writes[-1][1], b"V,0,0\n")
        self.assertGreater(port.writes[-1][0] - port.writes[-2][0], 0.5)
        self.assertLess(port.writes[-1][0] - port.writes[-2][0], 0.7)

    def test_missing_watchdog_fails_and_requests_stop(self):
        port = FakePort()
        with self.assertRaisesRegex(RuntimeError, "700 ms"):
            self.run_fake(port, watchdog=True)
        self.assertEqual(port.writes[-1][1], b"V,0,0\n")

    def test_stale_zero_feedback_cannot_pass_watchdog(self):
        port = FakePort()
        with (
            patch.object(serial_test.time, "monotonic", lambda: port.now),
            contextlib.redirect_stdout(io.StringIO()),
            self.assertRaisesRegex(RuntimeError, "missing active feedback"),
        ):
            serial_test.observe_watchdog(port, 0.0)

    def test_five_second_refresh_and_stop(self):
        port = FakePort()
        self.run_fake(port)
        times = [t for t, command in port.writes if command == b"V,100,100\n"]
        self.assertGreaterEqual(len(times), 45)
        self.assertLessEqual(len(times), 51)
        self.assertTrue(all(0.09 <= b - a <= 0.12 for a, b in pairwise(times)))
        self.assertAlmostEqual(port.writes[-1][0] - times[0], 5.0, delta=0.02)
        self.assertEqual(port.writes[-1][1], b"V,0,0\n")

    def test_missing_telemetry_prevents_motion(self):
        port = FakePort("no_telemetry")
        with self.assertRaisesRegex(RuntimeError, "Preflight"):
            self.run_fake(port)
        self.assertEqual([command for _, command in port.writes], [b"V,0,0\n"])

    def test_feedback_loss_and_wrong_direction_request_stop(self):
        for failure in ("disconnect", "reverse"):
            with self.subTest(failure=failure):
                port = FakePort(failure)
                with self.assertRaises(RuntimeError):
                    self.run_fake(port)
                self.assertEqual(port.writes[-1][1], b"V,0,0\n")

    def test_keyboard_interrupt_requests_active_stop(self):
        port = FakePort("interrupt")
        with self.assertRaises(KeyboardInterrupt):
            self.run_fake(port)
        self.assertEqual(port.writes[-1][1], b"V,0,0\n")


if __name__ == "__main__":
    unittest.main()
