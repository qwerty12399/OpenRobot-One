[CmdletBinding()]
param(
    [string]$Port = 'COM4',
    [int]$PingCount = 100,
    [int]$ReconnectCount = 10,
    [string]$ProgrammerCli = 'STM32_Programmer_CLI.exe'
)

$ErrorActionPreference = 'Stop'

if ($PingCount -lt 1) {
    throw 'PingCount must be at least 1.'
}
if ($ReconnectCount -lt 1) {
    throw 'ReconnectCount must be at least 1.'
}

function New-H2SerialPort {
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

function Reset-H2Board {
    $output = & $ProgrammerCli -c port=SWD mode=HOTPLUG -rst 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0 -or $output -notmatch 'reset is performed') {
        throw "STM32 reset failed: $output"
    }
}

function Read-H2Line([System.IO.Ports.SerialPort]$Serial) {
    return $Serial.ReadLine().TrimEnd("`r")
}

function Test-H2PartialTimeout {
    $serial = New-H2SerialPort
    try {
        $serial.Open()
        $serial.DiscardInBuffer()
        Reset-H2Board
        $ready = Read-H2Line $serial
        $serial.Write('H2-')
        Start-Sleep -Milliseconds 750
        $timeoutExtra = $serial.ReadExisting()
        $serial.Write("PING`r`n")
        $fragmentReply = Read-H2Line $serial
        $serial.Write("H2-PING`r`n")
        $recoveryReply = Read-H2Line $serial

        Write-Host "PARTIAL_TIMEOUT_READY=$ready"
        Write-Host "PARTIAL_TIMEOUT_EXTRA_BYTES=$([Text.Encoding]::ASCII.GetByteCount($timeoutExtra))"
        Write-Host "PARTIAL_FRAGMENT_REPLY=$fragmentReply"
        Write-Host "PARTIAL_RECOVERY_REPLY=$recoveryReply"

        return (($ready -eq 'H2-READY') -and
            ($timeoutExtra.Length -eq 0) -and
            ($fragmentReply -eq 'H2-ERR') -and
            ($recoveryReply -eq 'H2-PONG'))
    } finally {
        if ($serial.IsOpen) {
            $serial.Close()
        }
        $serial.Dispose()
    }
}

function Test-H2WrongBaudRecovery {
    $wrongBaud = [System.IO.Ports.SerialPort]::new(
        $Port,
        9600,
        [System.IO.Ports.Parity]::None,
        8,
        [System.IO.Ports.StopBits]::One
    )
    $wrongBaud.WriteTimeout = 1000
    $wrongBaud.DtrEnable = $false
    $wrongBaud.RtsEnable = $false
    try {
        $wrongBaud.Open()
        $wrongBaud.DiscardInBuffer()
        Reset-H2Board
        Start-Sleep -Milliseconds 200
        $wrongBaud.Write("H2-PING`r`n")
        Start-Sleep -Milliseconds 200
    } finally {
        if ($wrongBaud.IsOpen) {
            $wrongBaud.Close()
        }
        $wrongBaud.Dispose()
    }

    Start-Sleep -Milliseconds 750
    $serial = New-H2SerialPort
    try {
        $serial.Open()
        $serial.DiscardInBuffer()
        $serial.Write("H2-PING`r`n")
        $reply = Read-H2Line $serial
        Write-Host "WRONG_BAUD_RECOVERY_REPLY=$reply"
        return ($reply -eq 'H2-PONG')
    } finally {
        if ($serial.IsOpen) {
            $serial.Close()
        }
        $serial.Dispose()
    }
}

$failures = 0
$serial = New-H2SerialPort
try {
    $serial.Open()
    $serial.DiscardInBuffer()
    $serial.DiscardOutBuffer()
    Reset-H2Board

    $ready = Read-H2Line $serial
    if ($ready -ne 'H2-READY') {
        throw "Expected H2-READY, received '$ready'."
    }

    $pingPasses = 0
    for ($index = 1; $index -le $PingCount; $index++) {
        $serial.Write("H2-PING`r`n")
        $reply = Read-H2Line $serial
        if ($reply -eq 'H2-PONG') {
            $pingPasses++
        } else {
            $failures++
            Write-Output "PING_FAIL index=$index received=$reply"
        }
    }

    $serial.Write("UNKNOWN`r`n")
    $unknownReply = Read-H2Line $serial
    if ($unknownReply -ne 'H2-ERR') {
        $failures++
    }

    $serial.Write(('X' * 40) + "`r`n")
    $overlongReply = Read-H2Line $serial
    if ($overlongReply -ne 'H2-ERR') {
        $failures++
    }

    Start-Sleep -Milliseconds 200
    $extra = $serial.ReadExisting()
    if ($extra.Length -ne 0) {
        $failures++
    }

    Write-Output "READY=$ready"
    Write-Output "PING_PASS=$pingPasses PING_EXPECTED=$PingCount"
    Write-Output "UNKNOWN_REPLY=$unknownReply"
    Write-Output "OVERLONG_REPLY=$overlongReply"
    Write-Output "EXTRA_BYTES=$([Text.Encoding]::ASCII.GetByteCount($extra))"
} finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
    $serial.Dispose()
}

$partialTimeoutPass = Test-H2PartialTimeout
if (-not $partialTimeoutPass) {
    $failures++
}

$wrongBaudRecoveryPass = Test-H2WrongBaudRecovery
if (-not $wrongBaudRecoveryPass) {
    $failures++
}

Write-Output "PARTIAL_TIMEOUT_RESULT=$(if ($partialTimeoutPass) { 'PASS' } else { 'FAIL' })"
Write-Output "WRONG_BAUD_RECOVERY_RESULT=$(if ($wrongBaudRecoveryPass) { 'PASS' } else { 'FAIL' })"

$reconnectPasses = 0
for ($cycle = 1; $cycle -le $ReconnectCount; $cycle++) {
    $serial = New-H2SerialPort
    try {
        $serial.Open()
        $serial.DiscardInBuffer()
        Reset-H2Board
        $ready = Read-H2Line $serial
        $serial.Write("H2-PING`r`n")
        $reply = Read-H2Line $serial
        if ($ready -eq 'H2-READY' -and $reply -eq 'H2-PONG') {
            $reconnectPasses++
        } else {
            $failures++
            Write-Output "RECONNECT_FAIL cycle=$cycle ready=$ready reply=$reply"
        }
    } catch {
        $failures++
        Write-Output "RECONNECT_FAIL cycle=$cycle error=$($_.Exception.Message)"
    } finally {
        if ($serial.IsOpen) {
            $serial.Close()
        }
        $serial.Dispose()
    }
    Start-Sleep -Milliseconds 100
}

Write-Output "RECONNECT_PASS=$reconnectPasses RECONNECT_EXPECTED=$ReconnectCount"
Write-Output "TOTAL_FAILURES=$failures"
if ($failures -ne 0) {
    Write-Output 'H2_UART_RESULT=FAIL'
    exit 1
}

Write-Output 'H2_UART_RESULT=PASS'
