$ErrorActionPreference = 'Continue'
$logPath = Join-Path $PSScriptRoot 'module1-stage1.log'
Start-Transcript -LiteralPath $logPath -Force

Write-Host '=== Administrator check ==='
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator
)
if (-not $isAdmin) {
    throw 'This script must run as administrator.'
}

Write-Host '=== Stop and uninstall VMware Workstation 17.5.0 ==='
Get-Process -Name 'vmware','vmware-tray','vmware-authd','vmnat','vmnetdhcp' -ErrorAction SilentlyContinue |
    Stop-Process -Force -ErrorAction SilentlyContinue

$vmwareProductCode = '{00BF49FA-E6A3-4227-A18E-4A9036594E9D}'
$vmwareInstalled = Test-Path -LiteralPath "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$vmwareProductCode"
if (-not $vmwareInstalled) {
    $vmwareInstalled = Test-Path -LiteralPath "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\$vmwareProductCode"
}
if ($vmwareInstalled) {
    $uninstallProcess = Start-Process -FilePath 'msiexec.exe' -ArgumentList "/x $vmwareProductCode /qn /norestart" -Wait -PassThru
    Write-Host "VMware uninstaller exit code: $($uninstallProcess.ExitCode)"
}

$vmwareServices = @(
    'VMAuthdService',
    'VMnetDHCP',
    'VMware NAT Service',
    'VMUSBArbService',
    'VMwareHostd',
    'VmwareAutostartService',
    'vmkbd',
    'vmx86',
    'vmci',
    'vmnetbridge',
    'vmnetuserif',
    'hcmon'
)
foreach ($serviceName in $vmwareServices) {
    & sc.exe stop $serviceName 2>$null | Out-Null
    & sc.exe delete $serviceName 2>$null | Out-Null
}

Get-PnpDevice -Class Net -ErrorAction SilentlyContinue |
    Where-Object { $_.FriendlyName -match 'VMware|VMnet' } |
    ForEach-Object {
        Write-Host "Removing VMware network device: $($_.FriendlyName) [$($_.InstanceId)]"
        & pnputil.exe /remove-device $_.InstanceId
    }

$vmwareRegistryPaths = @(
    'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\VMware, Inc.',
    'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\VMware, Inc.',
    'Registry::HKEY_CURRENT_USER\SOFTWARE\VMware, Inc.'
)
foreach ($registryPath in $vmwareRegistryPaths) {
    if (Test-Path -LiteralPath $registryPath) {
        Remove-Item -LiteralPath $registryPath -Recurse -Force
    }
}

$vmwareFolders = @(
    (Join-Path $env:ProgramFiles 'VMware'),
    (Join-Path ${env:ProgramFiles(x86)} 'VMware'),
    (Join-Path $env:ProgramData 'VMware'),
    (Join-Path $env:LOCALAPPDATA 'VMware'),
    (Join-Path $env:APPDATA 'VMware')
) | Where-Object { $_ }

foreach ($folder in $vmwareFolders) {
    if (Test-Path -LiteralPath $folder) {
        $resolvedFolder = (Resolve-Path -LiteralPath $folder).Path
        $allowedRoots = @(
            [System.IO.Path]::GetFullPath($env:ProgramFiles),
            [System.IO.Path]::GetFullPath(${env:ProgramFiles(x86)}),
            [System.IO.Path]::GetFullPath($env:ProgramData),
            [System.IO.Path]::GetFullPath($env:LOCALAPPDATA),
            [System.IO.Path]::GetFullPath($env:APPDATA)
        ) | Where-Object { $_ }
        $insideAllowedRoot = $allowedRoots | Where-Object {
            $resolvedFolder.StartsWith($_ + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)
        }
        if ($insideAllowedRoot -and (Split-Path -Leaf $resolvedFolder) -eq 'VMware') {
            Write-Host "Removing verified VMware folder: $resolvedFolder"
            Remove-Item -LiteralPath $resolvedFolder -Recurse -Force
        }
    }
}

Write-Host '=== Enable WSL2 prerequisites ==='
Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux -All -NoRestart
Enable-WindowsOptionalFeature -Online -FeatureName VirtualMachinePlatform -All -NoRestart
& bcdedit.exe /set hypervisorlaunchtype auto

Write-Host '=== Update installed development applications ==='
& winget source update
& winget upgrade --id Git.Git -e --source winget --accept-package-agreements --accept-source-agreements --silent
& winget upgrade --id Microsoft.VisualStudioCode -e --source winget --accept-package-agreements --accept-source-agreements --silent
& winget upgrade --id Docker.DockerDesktop -e --source winget --accept-package-agreements --accept-source-agreements --silent

Write-Host '=== Install VS Code extensions ==='
$codeCommand = Get-Command code.cmd -ErrorAction SilentlyContinue
if ($codeCommand) {
    $extensions = @(
        'ms-vscode-remote.remote-wsl',
        'ms-vscode.cpptools-extension-pack',
        'ms-python.python',
        'ms-python.vscode-pylance',
        'ms-iot.vscode-ros',
        'ms-vscode.cmake-tools',
        'ms-azuretools.vscode-docker',
        'ms-vscode-remote.remote-containers'
    )
    foreach ($extension in $extensions) {
        & $codeCommand.Source --install-extension $extension --force
    }
}

Write-Host '=== Stage 1 verification ==='
Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux |
    Select-Object FeatureName, State
Get-WindowsOptionalFeature -Online -FeatureName VirtualMachinePlatform |
    Select-Object FeatureName, State
Get-Service -ErrorAction SilentlyContinue |
    Where-Object { $_.DisplayName -match 'VMware' -or $_.Name -match 'VMware|VMAuth|VMnet|VMUSB' } |
    Select-Object Name, DisplayName, Status
Get-NetAdapter -IncludeHidden -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match 'VMware|VMnet' -or $_.InterfaceDescription -match 'VMware|VMnet' } |
    Select-Object Name, InterfaceDescription, Status

Stop-Transcript
