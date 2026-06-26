param(
    [ValidateSet("Alone2", "Alone", "Alone7z", "Fm", "Gui", "Explorer", "Installer", "Uninstaller", "LzmaCon", "SFXCon", "SFXWin", "Setup", "All")]
    [string]$Target = "Alone2",

    [ValidateSet("x64", "x86", "arm64", "All")]
    [string]$Arch = "x64",

    [string]$OutputPath = "dist\releases\winzst-windows-amd64-installer.exe"
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$targetMap = @{
    "Alone2"      = "CPP\7zip\Bundles\Alone2"
    "Alone"       = "CPP\7zip\Bundles\Alone"
    "Alone7z"     = "CPP\7zip\Bundles\Alone7z"
    "Fm"          = "CPP\7zip\Bundles\Fm"
    "Gui"         = "CPP\7zip\UI\GUI"
    "Explorer"    = "CPP\7zip\UI\Explorer"
    "Installer"   = "C\Util\7zipInstall"
    "Uninstaller" = "C\Util\7zipUninstall"
    "LzmaCon"     = "CPP\7zip\Bundles\LzmaCon"
    "SFXCon"      = "CPP\7zip\Bundles\SFXCon"
    "SFXWin"      = "CPP\7zip\Bundles\SFXWin"
}

$supportedTargets = @("Alone2", "Alone", "Alone7z", "Fm", "Gui", "Explorer", "Installer", "Uninstaller", "LzmaCon", "SFXCon", "SFXWin")
$setupTargets = @("Installer", "Uninstaller", "Alone2", "Fm", "Gui", "Explorer")
$supportedArchitectures = @("x64", "x86", "arm64")

# Some 7-Zip makefiles produce secondary per-architecture output directories outside
# the directly invoked makefile directory. Keep these scoped to generated <arch> dirs.
$additionalArtifactDirectoriesByTarget = @{
    "Fm" = @("CPP\7zip\UI\FileManager")
}

$targetsToClean = if ($Target -eq "All") {
    $supportedTargets
}
elseif ($Target -eq "Setup") {
    $setupTargets
}
else {
    @($Target)
}

$architecturesToClean = if ($Arch -eq "All") {
    $supportedArchitectures
}
else {
    @($Arch)
}

function Remove-BuildArtifactPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (Test-Path -LiteralPath $Path) {
        Write-Host "Removing: $Path"
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
}

function Remove-EmptyDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if ((Test-Path -LiteralPath $Path -PathType Container) -and -not (Get-ChildItem -LiteralPath $Path -Force)) {
        Write-Host "Removing empty directory: $Path"
        Remove-Item -LiteralPath $Path -Force
    }
}

function Remove-EmptyAncestorDirectories {
    param(
        [Parameter(Mandatory = $true)]
        [string]$StartingDirectory,

        [Parameter(Mandatory = $true)]
        [string]$StopDirectory
    )

    $currentDirectory = $StartingDirectory
    $resolvedStopDirectory = (Resolve-Path -LiteralPath $StopDirectory).Path.TrimEnd('\')

    while (-not [string]::IsNullOrWhiteSpace($currentDirectory)) {
        if (-not (Test-Path -LiteralPath $currentDirectory -PathType Container)) {
            $currentDirectory = Split-Path -Parent $currentDirectory
            continue
        }

        $resolvedCurrentDirectory = (Resolve-Path -LiteralPath $currentDirectory).Path.TrimEnd('\')
        if ($resolvedCurrentDirectory -eq $resolvedStopDirectory) {
            break
        }

        if (Get-ChildItem -LiteralPath $currentDirectory -Force) {
            break
        }

        Write-Host "Removing empty directory: $currentDirectory"
        Remove-Item -LiteralPath $currentDirectory -Force
        $currentDirectory = Split-Path -Parent $currentDirectory
    }
}

function Remove-InstallerArtifacts {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Architectures,

        [Parameter(Mandatory = $true)]
        [string]$ReleaseOutputPath
    )

    foreach ($currentArch in $Architectures) {
        Remove-BuildArtifactPath -Path (Join-Path $repoRoot "dist\windows\$currentArch\installer-stage")
        Remove-BuildArtifactPath -Path (Join-Path $repoRoot "dist\windows\$currentArch\installer-payload.7z")
        Remove-EmptyDirectory -Path (Join-Path $repoRoot "dist\windows\$currentArch")
    }

    $releasePath = Join-Path $repoRoot $ReleaseOutputPath
    Remove-BuildArtifactPath -Path $releasePath

    $releaseDirectory = Split-Path -Parent $releasePath
    if (-not [string]::IsNullOrWhiteSpace($releaseDirectory)) {
        Remove-EmptyAncestorDirectories -StartingDirectory $releaseDirectory -StopDirectory $repoRoot
    }

    # Remove legacy/default release names that may have been produced by older versions of the
    # packaging script or by running Setup for a non-x64 architecture before OutputPath was fixed.
    foreach ($releaseName in @(
        "winzst-windows-amd64-installer.exe",
        "winzst-windows-x64-installer.exe",
        "winzst-windows-x86-installer.exe",
        "winzst-windows-arm64-installer.exe"
    )) {
        Remove-BuildArtifactPath -Path (Join-Path $repoRoot "dist\releases\$releaseName")
    }

    Remove-EmptyDirectory -Path (Join-Path $repoRoot "dist\releases")
    Remove-EmptyDirectory -Path (Join-Path $repoRoot "dist\windows")
    Remove-EmptyDirectory -Path (Join-Path $repoRoot "dist")
}

foreach ($currentTarget in $targetsToClean) {
    $targetDirectory = Join-Path $repoRoot $targetMap[$currentTarget]
    if (-not (Test-Path -LiteralPath $targetDirectory)) {
        throw "Build target directory was not found: '$targetDirectory'."
    }

    foreach ($currentArch in $architecturesToClean) {
        Invoke-InVsEnvironment `
            -WorkingDirectory $targetDirectory `
            -Command "nmake NEW_COMPILER=1 clean" `
            -Arch $currentArch

        # nmake clean can leave behind binaries, import libraries, debug databases,
        # resource outputs, and other per-architecture files. build.ps1 always writes
        # target artifacts under <target>\<arch>, so remove that directory too.
        Remove-BuildArtifactPath -Path (Join-Path $targetDirectory $currentArch)

        if ($additionalArtifactDirectoriesByTarget.ContainsKey($currentTarget)) {
            foreach ($additionalRelativePath in $additionalArtifactDirectoriesByTarget[$currentTarget]) {
                Remove-BuildArtifactPath -Path (Join-Path (Join-Path $repoRoot $additionalRelativePath) $currentArch)
            }
        }
    }
}

if ($Target -eq "Setup" -or $Target -eq "All") {
    Remove-InstallerArtifacts -Architectures $architecturesToClean -ReleaseOutputPath $OutputPath
}
