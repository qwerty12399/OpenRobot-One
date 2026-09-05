[CmdletBinding()]
param(
    [string]$Port = 'COM4',
    [ValidateSet('LEFT', 'RIGHT')]
    [string]$Side = 'LEFT',
    [ValidateSet('FWD', 'REV')]
    [string]$Direction = 'FWD',
    [switch]$AllowMotion
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function New-MotorSerialPort {
    $serial = [System.IO.Ports.SerialPort]::new(
        $Port,
        115200,
        [System.IO.Ports.Parity]::None,
        8,
        [System.IO.Ports.StopBits]::One
    )
    $serial.Handshake = [System.IO.Ports.Handshake]::None
    $serial.NewLine = "`n"
    $serial.ReadTimeout = 2000
    $serial.WriteTimeout = 1000
    $serial.DtrEnable = $false
    $serial.RtsEnable = $false
    return $serial
}

function Invoke-MotorCommand {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [string]$Command
    )

    $Serial.Write("$Command`r`n")
    $reply = $Serial.ReadLine().TrimEnd("`r")
    Write-Host "$Command -> $reply"
    return $reply
}

$serial = New-MotorSerialPort
try {
    $serial.Open()
    $serial.DiscardInBuffer()
    $serial.DiscardOutBuffer()

    $stopReply = Invoke-MotorCommand -Serial $serial -Command 'MOTOR-STOP'
    if ($stopReply -ne 'MOTOR-STOPPED') {
        throw "Expected MOTOR-STOPPED, received '$stopReply'."
    }

    $statusReply = Invoke-MotorCommand -Serial $serial -Command 'MOTOR-STATUS'
    if ($statusReply -notmatch '^MOTOR-STATUS STATE=IDLE LEFT=\d+ RIGHT=\d+$') {
        throw "Unexpected status response '$statusReply'."
    }

    $deniedReply = Invoke-MotorCommand -Serial $serial -Command "MOTOR-PULSE $Side $Direction"
    if ($deniedReply -ne 'MOTOR-DENIED') {
        throw "An unarmed pulse was not denied: '$deniedReply'."
    }

    if (-not $AllowMotion) {
        Write-Output 'SAFE_PROTOCOL_RESULT=PASS'
        Write-Output 'MOTION_NOT_REQUESTED=PASS'
        exit 0
    }

    Write-Warning "A single 20%/100ms $Side $Direction motor pulse will now be requested."
    $armReply = Invoke-MotorCommand -Serial $serial -Command 'MOTOR-ARM'
    if ($armReply -ne 'MOTOR-ARMED') {
        throw "Expected MOTOR-ARMED, received '$armReply'."
    }

    $doneReply = Invoke-MotorCommand -Serial $serial -Command "MOTOR-PULSE $Side $Direction"
    if ($doneReply -notmatch "^MOTOR-DONE SIDE=$Side DIR=$Direction START=\d+ END=\d+ DELTA=-?\d+$") {
        throw "Unexpected pulse completion response '$doneReply'."
    }

    $finalStatus = Invoke-MotorCommand -Serial $serial -Command 'MOTOR-STATUS'
    if ($finalStatus -notmatch '^MOTOR-STATUS STATE=IDLE LEFT=\d+ RIGHT=\d+$') {
        throw "Motor did not return to IDLE: '$finalStatus'."
    }

    Write-Output 'MOTOR_PULSE_RESULT=PASS'
} finally {
    if ($serial.IsOpen) {
        try {
            $serial.Write("MOTOR-STOP`r`n")
        } catch {
            Write-Warning "Unable to send final MOTOR-STOP: $($_.Exception.Message)"
        }
        $serial.Close()
    }
    $serial.Dispose()
}
