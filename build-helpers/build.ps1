param(
    [ValidateSet("Alone2", "Alone", "Alone7z", "Format7zF", "Fm", "Gui", "Explorer", "Installer", "Uninstaller", "LzmaCon", "SFXCon", "SFXWin", "Setup")]
    [string]$Target = "Alone2",

    [ValidateSet("x64", "x86", "arm64")]
    [string]$Arch = "x64",

    [bool]$Static = $true,

    [switch]$CleanFirst,

    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$targetMap = @{
    "Alone2"  = @{ RelativePath = "CPP\7zip\Bundles\Alone2";  Output = "winzst.exe" }
    "Alone"   = @{ RelativePath = "CPP\7zip\Bundles\Alone";   Output = "7za.exe" }
    "Alone7z" = @{ RelativePath = "CPP\7zip\Bundles\Alone7z"; Output = "7zr.exe" }
    "Format7zF" = @{ RelativePath = "CPP\7zip\Bundles\Format7zF"; Output = "7z.dll" }
    "Fm"      = @{ RelativePath = "CPP\7zip\Bundles\Fm";      Output = "WinZSTFM.exe" }
    "Gui"     = @{ RelativePath = "CPP\7zip\UI\GUI";          Output = "WinZSTG.exe" }
    "Explorer" = @{ RelativePath = "CPP\7zip\UI\Explorer";    Output = "7-zip.dll" }
    "Installer" = @{ RelativePath = "C\Util\7zipInstall";     Output = "7zipInstall.exe" }
    "Uninstaller" = @{ RelativePath = "C\Util\7zipUninstall"; Output = "7zipUninstall.exe" }
    "LzmaCon" = @{ RelativePath = "CPP\7zip\Bundles\LzmaCon"; Output = "lzma.exe" }
    "SFXCon"  = @{ RelativePath = "CPP\7zip\Bundles\SFXCon";  Output = "7zCon.sfx" }
    "SFXWin"  = @{ RelativePath = "CPP\7zip\Bundles\SFXWin";  Output = "7z.sfx" }
}

function Invoke-BuildTarget {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildTarget
    )

    $targetInfo = $targetMap[$BuildTarget]
    $targetDirectory = Join-Path $repoRoot $targetInfo.RelativePath

    if (-not (Test-Path -LiteralPath $targetDirectory)) {
        throw "Build target directory was not found: '$targetDirectory'."
    }

    $nmakeArguments = @("nmake", "NEW_COMPILER=1")
    if ($Static) {
        $nmakeArguments += "MY_STATIC_LINK=1"
    }

    if ($CleanFirst) {
        $cleanCommand = ($nmakeArguments + "clean") -join " "
        Invoke-InVsEnvironment -WorkingDirectory $targetDirectory -Command $cleanCommand -Arch $Arch
    }

    $buildCommand = ($nmakeArguments + "all") -join " "
    Invoke-InVsEnvironment -WorkingDirectory $targetDirectory -Command $buildCommand -Arch $Arch

    $outputDirectory = Join-Path $targetDirectory $Arch
    Write-Host "Expected output directory: $outputDirectory"
    $expectedOutput = Join-Path $outputDirectory $targetInfo.Output
    Write-Host "Expected output: $expectedOutput"

    if (Test-Path -LiteralPath $outputDirectory) {
        Get-ChildItem -LiteralPath $outputDirectory | Format-Table -AutoSize
    }

    if (-not (Test-Path -LiteralPath $expectedOutput)) {
        throw "Expected build output was not found: '$expectedOutput'."
    }

    return $expectedOutput
}

if ($Target -eq "Setup") {
    $setupDependencies = @("Installer", "Uninstaller", "Alone2", "Format7zF", "Fm", "Gui", "Explorer")
    foreach ($dependency in $setupDependencies) {
        Invoke-BuildTarget -BuildTarget $dependency | Out-Null
    }

    $packageScript = Join-Path $repoRoot "build-helpers\package-installer.ps1"
    $packageArguments = @{
        Arch = $Arch
    }
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        $packageArguments.OutputPath = $OutputPath
    }

    & $packageScript @packageArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Packaging the WinZST setup executable failed with exit code $LASTEXITCODE."
    }
}
else {
    Invoke-BuildTarget -BuildTarget $Target | Out-Null
}
