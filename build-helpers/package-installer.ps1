param(
    [ValidateSet("x64", "x86", "arm64")]
    [string]$Arch = "x64",

    [string]$OutputPath = "dist\releases\winzst-windows-amd64-installer.exe"
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$releasePath = Join-Path $repoRoot $OutputPath
$releaseDir = Split-Path -Parent $releasePath
$stageRoot = Join-Path $repoRoot "dist\windows\$Arch\installer-stage"
$payloadArchive = Join-Path $repoRoot "dist\windows\$Arch\installer-payload.7z"
$installerStub = Join-Path $repoRoot "C\Util\7zipInstall\$Arch\7zipInstall.exe"
$uninstallerStub = Join-Path $repoRoot "C\Util\7zipUninstall\$Arch\7zipUninstall.exe"
$cliExe = Join-Path $repoRoot "CPP\7zip\Bundles\Alone2\$Arch\winzst.exe"

$requiredFiles = @(
    @{ Source = $installerStub; Name = "7zipInstall.exe" },
    @{ Source = $uninstallerStub; Name = "Uninstall.exe" },
    @{ Source = (Join-Path $repoRoot "CPP\7zip\Bundles\Alone2\$Arch\winzst.exe"); Name = "winzst.exe" },
    @{ Source = (Join-Path $repoRoot "CPP\7zip\Bundles\Fm\$Arch\WinZSTFM.exe"); Name = "WinZSTFM.exe" },
    @{ Source = (Join-Path $repoRoot "CPP\7zip\UI\GUI\$Arch\WinZSTG.exe"); Name = "WinZSTG.exe" }
)

foreach ($file in $requiredFiles) {
    if (-not (Test-Path -LiteralPath $file.Source)) {
        throw "Required installer input was not found: '$($file.Source)'."
    }
}

if (-not (Test-Path -LiteralPath $cliExe)) {
    throw "Required archiver executable was not found: '$cliExe'."
}

Remove-Item $stageRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $payloadArchive -Force -ErrorAction SilentlyContinue
Remove-Item $releasePath -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null

foreach ($file in $requiredFiles) {
    if ($file.Name -eq "7zipInstall.exe") {
        continue
    }
    Copy-Item -LiteralPath $file.Source -Destination (Join-Path $stageRoot $file.Name) -Force
}

$optionalFiles = @(
    @{ Source = (Join-Path $repoRoot "CPP\7zip\UI\Explorer\$Arch\7-zip.dll"); Name = "7-zip.dll" },
    @{ Source = (Join-Path $repoRoot "CPP\7zip\UI\Explorer\$Arch\7-zip32.dll"); Name = "7-zip32.dll" },
    @{ Source = (Join-Path $repoRoot "CPP\7zip\UI\FileManager\$Arch\7z.dll"); Name = "7z.dll" },
    @{ Source = (Join-Path $repoRoot "CPP\7zip\Bundles\SFXWin\$Arch\7z.sfx"); Name = "7z.sfx" },
    @{ Source = (Join-Path $repoRoot "CPP\7zip\Bundles\SFXCon\$Arch\7zCon.sfx"); Name = "7zCon.sfx" },
    @{ Source = (Join-Path $repoRoot "DOC\License.txt"); Name = "License.txt" },
    @{ Source = (Join-Path $repoRoot "DOC\src-history.txt"); Name = "History.txt" },
    @{ Source = (Join-Path $repoRoot "README.md"); Name = "README.md" }
)

foreach ($file in $optionalFiles) {
    if (Test-Path -LiteralPath $file.Source) {
        Copy-Item -LiteralPath $file.Source -Destination (Join-Path $stageRoot $file.Name) -Force
    }
}

Write-Host "Installer stage contents:"
Get-ChildItem -LiteralPath $stageRoot | Select-Object Name, Length | Format-Table -AutoSize

& $cliExe a -t7z -m0=lzma -mf=off $payloadArchive (Join-Path $stageRoot '*')
if ($LASTEXITCODE -ne 0) {
    throw "Creating installer payload archive failed with exit code $LASTEXITCODE."
}

$stubBytes = [System.IO.File]::OpenRead($installerStub)
$payloadBytes = [System.IO.File]::OpenRead($payloadArchive)
$outputBytes = [System.IO.File]::Create($releasePath)

try {
    $stubBytes.CopyTo($outputBytes)
    $payloadBytes.CopyTo($outputBytes)
}
finally {
    $outputBytes.Dispose()
    $payloadBytes.Dispose()
    $stubBytes.Dispose()
}

if (-not (Test-Path -LiteralPath $releasePath)) {
    throw "Installer artifact was not created: '$releasePath'."
}

Get-Item -LiteralPath $releasePath | Format-List FullName, Length, LastWriteTime
