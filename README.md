# WinZST

WinZST is a Windows archive utility derived from 7-Zip. It focuses on making compressed tarballs behave like normal archives on Windows: when a user extracts `example.tar.zst`, `example.tzs`, `example.tgz`, or `example.txz`, the expected output is the archive contents, not an intermediate `.tar` file.

The preferred WinZST archive format is `.tzs`.

```text
.tzs == .tar.zst
```

`.tzs` is not a proprietary container. It is a standard tar archive compressed with Zstandard, intended to make tar plus Zstandard feel natural in Windows file managers and Explorer workflows.

## Project Status

This repository is an early WinZST fork of the 7-Zip source tree. The first target platform is Windows 11 x64. Early builds may be experimental and unsigned, so Windows SmartScreen or similar trust prompts should be expected until release signing is established.

The current implementation exposes `.tzs` creation through the inherited archive pipeline and writes valid Zstandard frames using stored/raw blocks. That keeps `.tzs` output standards-compatible without adding a new dependency, but it is not a final compression-ratio implementation. A full Zstandard encoder should replace this temporary stored-frame writer when the encoder dependency is added.

## Archive Behavior

WinZST should keep broad archive support inherited from 7-Zip, including ZIP, 7z, tar formats, compressed tar formats, and extraction-only RAR support where the inherited source and licenses allow it.

Primary behavior goals:

- Prefer `.tzs` for new archives.
- Support `.tar.zst` as the expanded equivalent of `.tzs`.
- Extract compressed tarballs directly to their final contents.
- Keep `.zip` and `.7z` creation available for compatibility.
- Preserve upstream copyright, LGPL, unRAR, and third-party notices.
- Avoid using 7-Zip branding as WinZST branding.

## Building

WinZST uses the inherited upstream 7-Zip NMAKE build. The PowerShell helper scripts in `build-helpers/` wrap that existing build flow to make it easier to use from the repository root; they intentionally do not replace the upstream build system.

Requirements:

- Windows
- PowerShell
- Visual Studio 2022 Build Tools
- MSVC C++ toolchain
- Windows SDK

Default build command:

```powershell
.\build-helpers\build.ps1
```

Clean rebuild command:

```powershell
.\build-helpers\build.ps1 -CleanFirst
```

Build File Manager command:

```powershell
.\build-helpers\build.ps1 -Target Fm
```

Clean all helper-supported targets:

```powershell
.\build-helpers\clean.ps1 -Target All
```

The intended first-class build target is Windows 11 x64. Other inherited build targets may continue to work, but they are not the initial product target.

## Security Notes

Archive extraction handles untrusted input and writes to the filesystem. WinZST must preserve inherited path safety behavior and should prefer safe Windows extraction semantics over perfect Unix metadata restoration.

Do not weaken checks for path traversal, absolute paths, unsafe links, alternate streams, overwrites, plugin loading, password handling, or archive parser behavior for compatibility convenience.
