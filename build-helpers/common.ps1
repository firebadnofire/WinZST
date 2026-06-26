$ErrorActionPreference = "Stop"

function Get-RepoRoot {
    $scriptDir = Split-Path -Parent $PSScriptRoot
    return (Resolve-Path -LiteralPath $scriptDir).Path
}

function Get-VsWherePath {
    $vsWherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vsWherePath)) {
        throw "vswhere.exe was not found at '$vsWherePath'. Install Visual Studio 2022 Build Tools."
    }

    return $vsWherePath
}

function Get-VisualStudioInstallationPath {
    $vsWherePath = Get-VsWherePath
    $installationPath = & $vsWherePath `
        -latest `
        -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -version "[17.0,18.0)" `
        -property installationPath

    if ($LASTEXITCODE -ne 0) {
        throw "vswhere failed with exit code $LASTEXITCODE."
    }

    if ([string]::IsNullOrWhiteSpace($installationPath)) {
        throw "No Visual Studio 2022 installation with MSVC x86/x64 tools was found."
    }

    return $installationPath.Trim()
}

function Get-VcVarsBatchPath {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("x64", "x86", "arm64")]
        [string]$Arch
    )

    $installationPath = Get-VisualStudioInstallationPath
    $relativePath = switch ($Arch) {
        "x64" { "VC\Auxiliary\Build\vcvars64.bat" }
        "x86" { "VC\Auxiliary\Build\vcvars32.bat" }
        "arm64" { "VC\Auxiliary\Build\vcvarsamd64_arm64.bat" }
        default { throw "Unsupported architecture '$Arch'." }
    }

    $batchPath = Join-Path $installationPath $relativePath
    if (-not (Test-Path -LiteralPath $batchPath)) {
        throw "Visual Studio environment batch file was not found at '$batchPath'."
    }

    return $batchPath
}

function Invoke-InVsEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkingDirectory,

        [Parameter(Mandatory = $true)]
        [string]$Command,

        [Parameter(Mandatory = $true)]
        [ValidateSet("x64", "x86", "arm64")]
        [string]$Arch
    )

    $resolvedWorkingDirectory = (Resolve-Path -LiteralPath $WorkingDirectory).Path
    $vcVarsBatchPath = Get-VcVarsBatchPath -Arch $Arch

    Write-Host "Working directory: $resolvedWorkingDirectory"
    Write-Host "Command: $Command"

    $cmdScript = "`"$vcVarsBatchPath`" && cd /d `"$resolvedWorkingDirectory`" && $Command"
    & cmd.exe /c $cmdScript

    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE."
    }
}
