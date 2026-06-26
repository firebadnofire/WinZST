param(
    [ValidateSet("Alone2", "Alone", "Alone7z", "Fm", "Gui", "Explorer", "Installer", "Uninstaller", "LzmaCon", "SFXCon", "SFXWin", "All")]
    [string]$Target = "Alone2",

    [ValidateSet("x64", "x86", "arm64")]
    [string]$Arch = "x64"
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$targetMap = @{
    "Alone2"     = "CPP\7zip\Bundles\Alone2"
    "Alone"      = "CPP\7zip\Bundles\Alone"
    "Alone7z"    = "CPP\7zip\Bundles\Alone7z"
    "Fm"         = "CPP\7zip\Bundles\Fm"
    "Gui"        = "CPP\7zip\UI\GUI"
    "Explorer"   = "CPP\7zip\UI\Explorer"
    "Installer"  = "C\Util\7zipInstall"
    "Uninstaller"= "C\Util\7zipUninstall"
    "LzmaCon"    = "CPP\7zip\Bundles\LzmaCon"
    "SFXCon"     = "CPP\7zip\Bundles\SFXCon"
    "SFXWin"     = "CPP\7zip\Bundles\SFXWin"
}
$supportedTargets = @("Alone2", "Alone", "Alone7z", "Fm", "Gui", "Explorer", "Installer", "Uninstaller", "LzmaCon", "SFXCon", "SFXWin")
$targetsToClean = if ($Target -eq "All") { $supportedTargets } else { @($Target) }

foreach ($currentTarget in $targetsToClean) {
    $targetDirectory = Join-Path $repoRoot $targetMap[$currentTarget]
    if (-not (Test-Path -LiteralPath $targetDirectory)) {
        throw "Build target directory was not found: '$targetDirectory'."
    }

    Invoke-InVsEnvironment `
        -WorkingDirectory $targetDirectory `
        -Command "nmake NEW_COMPILER=1 clean" `
        -Arch $Arch
}
