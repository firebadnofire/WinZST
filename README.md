# WinZST

WinZST is a Windows archive utility derived from 7-Zip. It focuses on making tar archives and compressed tar archives behave like normal Windows archives.

The core idea is simple:

```text
compressed tarball in -> final contents out
```

When a user extracts `example.tgz`, `example.txz`, `example.tbz`, `example.tar.zst`, or `example.tzs`, the expected output is the archive contents, not an intermediate `.tar` file.

WinZST is tarball-first, not Zstandard-only. Zstandard-compressed tarballs are the preferred default, but WinZST also supports other tar workflows such as gzip, bzip2, XZ/LZMA2, GNU tar, and POSIX tar, while keeping broad archive support inherited from 7-Zip.

## Why WinZST Exists

Compressed tarballs are common, portable, and useful, but Windows archive tools often expose their internal layering in awkward ways.

A user who downloads this:

```text
project.tar.gz
```

usually wants this:

```text
project/
```

not this:

```text
project.tar
```

WinZST exists to make tarballs worth downloading on Windows.

## Archive Formats

WinZST keeps the inherited general archive utility model, but gives tar formats better first-class treatment.

Primary creation formats include:

| Format | Typical extension | Notes |
| --- | --- | --- |
| WinZST archive | `.tzs` | Standard tar archive compressed with Zstandard |
| Tarball with Zstandard | `.tar.zst` | Expanded form of `.tzs` |
| Tarball with gzip | `.tgz`, `.tar.gz` | Common Unix/Linux tarball format |
| Tarball with bzip2 | `.tbz`, `.tbz2`, `.tar.bz2` | Older but still common compressed tar format |
| Tarball with XZ/LZMA2 | `.txz`, `.tar.xz` | High-compression tarball format |
| GNU tar | `.tar` | Plain tar archive using GNU tar behavior where supported |
| POSIX tar | `.tar` | Plain tar archive using POSIX tar behavior where supported |
| ZIP archive | `.zip` | Standard Windows-compatible archive format |
| 7-Zip archive | `.7z` | Inherited compatibility format |

Other inherited archive formats may remain supported where licensing, maintenance, and security allow.

## The `.tzs` Format

`.tzs` is WinZST's preferred archive extension.

```text
.tzs == .tar.zst
```

A `.tzs` file is a standard tar archive compressed with Zstandard.

It is not:

- a proprietary container
- a renamed `.7z` archive
- a renamed `.zip` archive
- a WinZST-only format
- encrypted by default

A `.tzs` archive should remain compatible with standard tar plus Zstandard tooling. In practice, this means a user should be able to rename:

```text
archive.tzs
```

to:

```text
archive.tar.zst
```

and extract it with tools that understand tar and Zstandard.

The purpose of `.tzs` is Windows usability: a short, recognizable extension for a standard tar plus Zstandard archive.

## Tarball Extraction Behavior

WinZST should extract compressed tarballs directly to their final contents.

For example, if this archive:

```text
example.tgz
```

contains:

```text
example/
example/file.txt
```

then normal extraction should produce:

```text
example/
example/file.txt
```

It should not produce:

```text
example.tar
```

This behavior applies to supported compressed tarball formats, including:

```text
.tzs
.tar.zst
.tgz
.tar.gz
.tbz
.tbz2
.tar.bz2
.txz
.tar.xz
```

The tar layer is an implementation detail during normal extraction.

## Decompress-Only Behavior

WinZST may support decompress-only behavior for advanced users.

For example:

```text
example.tgz -> example.tar
example.txz -> example.tar
example.tzs -> example.tar
```

That is valid when the user explicitly asks for the raw tar stream.

However, decompress-only is not the default behavior. Normal archive extraction should give the user the final files and folders.

## Project Status

WinZST is an early fork of the 7-Zip source tree.

The first target platform is:

```text
Windows 11 x64
```

Early builds may be experimental and unsigned. Windows SmartScreen or similar trust prompts should be expected until release signing is established.

The current `.tzs` implementation writes standard-compatible Zstandard output through the inherited archive pipeline. If the current encoder path uses stored/raw Zstandard blocks, that should be treated as a temporary compatibility implementation, not the final compression-ratio target. A full Zstandard encoder should replace it when the dependency and build integration are ready.

## Relationship to 7-Zip

WinZST is derived from 7-Zip.

That inheritance is important. The existing 7-Zip codebase provides mature archive parsing, extraction, compression, Windows GUI infrastructure, shell integration, build rules, and broad format support.

WinZST is not official 7-Zip and is not endorsed by upstream 7-Zip.

WinZST must preserve applicable upstream copyright notices, LGPL notices, third-party notices, and unRAR license restrictions.

## RAR Support

WinZST may keep inherited RAR extraction support where the source tree and licenses allow it.

RAR support is extraction-only.

WinZST does not create `.rar` archives and must not use unRAR-derived code to recreate the proprietary RAR compression algorithm.

## Building

WinZST uses the inherited upstream 7-Zip NMAKE build system.

The PowerShell helper scripts in `build-helpers/` wrap that build flow from the repository root. They do not replace the upstream build system.

Requirements:

- Windows
- PowerShell
- Visual Studio 2022 Build Tools
- MSVC C++ toolchain
- Windows SDK

Default build:

```powershell
.\build-helpers\build.ps1
```

Clean rebuild:

```powershell
.\build-helpers\build.ps1 -CleanFirst
```

Build File Manager:

```powershell
.\build-helpers\build.ps1 -Target Fm
```

Clean helper-supported targets:

```powershell
.\build-helpers\clean.ps1 -Target All
```

If PowerShell blocks the helper scripts because they are unsigned, run them with an explicit temporary execution policy bypass:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-helpers\build.ps1
```

or:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-helpers\clean.ps1 -Target All
```

## Testing Expectations

A build is not useful just because it compiles.

At minimum, WinZST should be tested for:

- opening archives
- listing archive contents
- creating `.tzs`
- creating `.tgz`
- creating `.tbz`
- creating `.txz`
- creating plain `.tar`
- extracting `.tzs` directly to contents
- extracting `.tar.zst` directly to contents
- extracting `.tgz` and `.tar.gz` directly to contents
- extracting `.tbz`, `.tbz2`, and `.tar.bz2` directly to contents
- extracting `.txz` and `.tar.xz` directly to contents
- keeping `.zip` extraction and creation working
- keeping `.7z` extraction and creation working
- testing archives without extraction
- preserving safe extraction behavior

A WinZST build that creates `.tzs` but breaks ZIP or 7z is not good enough.

A WinZST build that still extracts compressed tarballs to intermediate `.tar` files by default is not good enough.

## Security Notes

Archive extraction handles untrusted input and writes to the filesystem.

WinZST must preserve safe extraction behavior and should prefer Windows-safe tar extraction over perfect Unix metadata restoration.

Do not weaken checks for:

- path traversal
- absolute paths
- drive-qualified Windows paths
- UNC paths
- unsafe symlinks
- unsafe hardlinks
- alternate data streams
- unsafe overwrites
- password handling
- plugin loading
- archive parser behavior

Tar metadata is not harmless. Symlinks, hardlinks, device files, owners, groups, permissions, and unusual paths all need careful handling on Windows.

Safety beats compatibility.

## Non-Goals

WinZST is not trying to be:

- a new proprietary archive format
- a replacement compression algorithm
- a cloud storage product
- an Electron rewrite
- a Rust rewrite
- a cosmetic 7-Zip reskin
- a RAR creator
- a fork that hides its upstream origin

The goal is narrower and more practical:

```text
Make tarballs behave correctly on Windows.
```

## License

WinZST is derived from 7-Zip and retains the applicable upstream license structure.

The source tree includes LGPL-covered code, third-party code under other compatible terms, and unRAR-derived code with additional restrictions.

Do not remove license files or upstream notices.

See the included license documents for details.
