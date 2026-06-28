param(
    [ValidateSet("x64", "x86", "arm64")]
    [string]$Arch = "x64",

    [bool]$Static = $true,

    [switch]$CleanFirst,

    [string]$OutputPath,

    [switch]$KeepStage
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$buildScript = Join-Path $repoRoot "build-helpers\build.ps1"

$portableTargets = @(
    @{
        Target = "Alone2"
        Source = "CPP\7zip\Bundles\Alone2\$Arch\winzst.exe"
        Name   = "winzst.exe"
    },
    @{
        Target = "Fm"
        Source = "CPP\7zip\Bundles\Fm\$Arch\WinZSTFM.exe"
        Name   = "WinZSTFM.exe"
    },
    @{
        Target = "Gui"
        Source = "CPP\7zip\UI\GUI\$Arch\WinZSTG.exe"
        Name   = "WinZSTG.exe"
    }
)

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = "dist\releases\winzst-windows-$Arch-portable.zip"
}

$releasePath = Join-Path $repoRoot $OutputPath
$releaseDir = Split-Path -Parent $releasePath
$stageRoot = Join-Path $repoRoot "dist\releases\portable-stage\$Arch"

New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null

foreach ($item in $portableTargets) {
    $buildArgs = @{
        Target = $item.Target
        Arch   = $Arch
        Static = $Static
    }

    if ($CleanFirst) {
        $buildArgs.CleanFirst = $true
    }

    & $buildScript @buildArgs

    if ($LASTEXITCODE -ne 0) {
        throw "Building target '$($item.Target)' failed with exit code $LASTEXITCODE."
    }
}

Remove-Item -LiteralPath $stageRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $releasePath -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null

foreach ($item in $portableTargets) {
    $sourcePath = Join-Path $repoRoot $item.Source

    if (-not (Test-Path -LiteralPath $sourcePath)) {
        throw "Required portable build output was not found: '$sourcePath'."
    }

    Copy-Item `
        -LiteralPath $sourcePath `
        -Destination (Join-Path $stageRoot $item.Name) `
        -Force
}

$optionalFiles = @(
    @{ Source = "DOC\License.txt"; Name = "License.txt" },
    @{ Source = "README.md"; Name = "README.md" }
)

foreach ($file in $optionalFiles) {
    $sourcePath = Join-Path $repoRoot $file.Source

    if (Test-Path -LiteralPath $sourcePath) {
        Copy-Item `
            -LiteralPath $sourcePath `
            -Destination (Join-Path $stageRoot $file.Name) `
            -Force
    }
}

Write-Host "Portable stage contents:"
Get-ChildItem -LiteralPath $stageRoot | Select-Object Name, Length | Format-Table -AutoSize

Compress-Archive `
    -Path (Join-Path $stageRoot '*') `
    -DestinationPath $releasePath `
    -CompressionLevel Optimal `
    -Force

if (-not (Test-Path -LiteralPath $releasePath)) {
    throw "Portable archive was not created: '$releasePath'."
}

if (-not $KeepStage) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}

Get-Item -LiteralPath $releasePath | Format-List FullName, Length, LastWriteTime