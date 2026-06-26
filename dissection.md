# Program Dissection

## 1. Executive Summary

This repository is a source snapshot/fork of 7-Zip 26.01, a file archiver primarily targeting Windows but with supported command-line builds for Linux and macOS. The top-level `README.md` points to the 7-Zip website, while `DOC/readme.txt` identifies the package as "7-Zip 26.01 Sources" and describes the source tree layout.

The program solves archive creation, extraction, listing, testing, hashing, GUI file management, Windows shell integration, self-extracting archive support, and format/plugin hosting. It is implemented in C, C++, Windows resource files, legacy Visual Studio project files, makefiles, and architecture-specific assembly.

For forking, this repo is suitable if the intended fork is a 7-Zip-derived archiver, a Zstandard-focused 7-Zip variant, a packaging/build fork, or a source base for archive format work. It is less suitable as a small reusable library without cleanup because the codebase is large, macro-heavy, COM-like, and tightly organized around 7-Zip's build and plugin conventions.

## 2. Repository State

- Current branch: `main`, tracking `origin/main`, from `git status --short --branch`.
- Working tree: clean before this document was added.
- Remote: `origin` points to `ssh://git@pubcode.archuser.org:2222/firebadnofire/WinZST.git`.
- Latest visible commits:
  - `8c63d71 26.01`
  - `839151e 26.00`
  - `5e96a82 25.01`
  - `3951499 25.00`
  - `e5431fa 24.09`
  - history continues through `f19f813 '21.07'` and `98e06a5 Initial commit`.
- Detected languages: C (`C/`), C++ (`CPP/`), assembly (`Asm/`), Windows resource scripts (`*.rc`), makefiles, legacy Visual Studio `.dsp`/`.dsw` projects, WiX installer metadata (`DOC/7zip.wxs`), and plain-text docs.
- Version marker: `C/7zVersion.h` defines version `26.01` and date `2026-04-27`.
- Approximate size: `rg --files` found 1,293 tracked files; 1,011 are C/C++/header/assembly source files.
- Build system: makefile-based. Windows builds use `nmake` from directories such as `CPP/7zip` or individual bundle folders. Unix-like builds use `make -f makefile.gcc` or wrapper makefiles such as `CPP/7zip/cmpl_gcc.mak`.
- Package managers: none detected. There are no `package.json`, `Cargo.toml`, `CMakeLists.txt`, `vcpkg.json`, `conanfile`, or similar manifests in the inspected top-level tree.
- CI system: none detected. There is no `.github` directory or visible YAML CI config.
- Test framework: no conventional unit/integration test framework detected. The repo contains sample/test utilities such as `C/Util/7z/7zMain.c` and user-facing archive test commands (`7z t`), but no standalone automated test suite was found.
- Local build environment constraint: in this PowerShell shell, `nmake`, `cl`, `make`, `gcc`, and `clang` were not on PATH, so no build/test command was executed.
- License: mostly GNU LGPL 2.1-or-later, with exceptions and mixed terms documented in `DOC/License.txt`. Important exceptions include unRAR-restricted RAR decompression code, BSD 3-clause LZFSE/ZSTD code, BSD 2-clause XXH64 code, and public-domain files where stated.
- Fork implications: a fork must preserve LGPL obligations and handle the unRAR restriction carefully. `DOC/readme.txt` documents `DISABLE_RAR=1` and `DISABLE_RAR_COMPRESS=1` build knobs for excluding RAR-related code in GCC builds.
- Activity/staleness: the visible git history tracks recent 7-Zip releases through 26.01, so this snapshot does not look abandoned. It does look like a direct source import rather than a modernized project: no CI, no generated project files for modern IDEs beyond legacy `.dsp`/`.dsw`, and no package metadata.

## 3. Top-Level File Inventory

- `README.md`: minimal GitHub-facing note pointing to `https://7-zip.org`.
- `DOC/`: project documentation, licenses, history, file format docs, method IDs, WiX installer script, and help project metadata.
- `C/`: ANSI C core libraries, codec implementations, portable utility code, LZMA SDK-style utilities, installer/uninstaller utilities, and Zstandard decoder code.
- `CPP/`: main C++ source tree for 7-Zip interfaces, archive handlers, compression codecs, crypto, Windows wrappers, UIs, plugins, and executable bundles.
- `Asm/`: architecture-specific optimized assembly for CRC, SHA, AES, LZMA, sorting, and related hot paths.
- `.git/`: repository metadata.
- `dissection.md`: this generated analysis document.

## 4. Architecture Overview

The architecture is the standard 7-Zip split between format handlers, compression/crypto codecs, shared stream/file abstractions, and user-facing shells.

Entry points:

- Console executable: `CPP/7zip/UI/Console/MainAr.cpp` defines `main`, establishes global stdout/stderr streams, checks CPU ISA support, calls `My_SetDefaultDllDirectories()` on Windows, then invokes `Main2` in `CPP/7zip/UI/Console/Main.cpp`.
- Console command dispatcher: `CPP/7zip/UI/Console/Main.cpp` parses commands and routes to benchmark, list, extract, update, hash, and info operations.
- GUI executable: `CPP/7zip/UI/GUI/GUI.cpp` is the GUI frontend for dialogs such as extract/compress/benchmark.
- File manager executable: `CPP/7zip/UI/FileManager/FM.cpp` and `CPP/7zip/UI/FileManager/App.cpp` implement the full Windows file manager.
- Shell extension: `CPP/7zip/UI/Explorer/` builds `7-zip.dll` shell integration.
- SFX modules: `CPP/7zip/Bundles/SFXCon/`, `SFXWin/`, and `SFXSetup/` build self-extracting archive stubs.
- DLL/bundle exports: `CPP/7zip/Archive/DllExports.cpp` and `CPP/7zip/Archive/ArchiveExports.cpp` expose archive handler creation and format metadata.

Core modules:

- `CPP/7zip/Archive/`: archive format handlers. Examples: `7z/`, `Zip/`, `Rar/`, `Tar/`, `Wim/`, `Cab/`, plus many single-file handlers such as `ZstdHandler.cpp`, `XzHandler.cpp`, `GzHandler.cpp`, and `SquashfsHandler.cpp`.
- `CPP/7zip/Compress/`: C++ codec wrappers and implementations. Examples: `LzmaDecoder.cpp`, `LzmaEncoder.cpp`, `BZip2*`, `Deflate*`, `Xz*`, `ZstdDecoder.cpp`, and RAR decoders.
- `C/`: C codec engines and low-level utilities. Examples: `C/ZstdDec.c`, `C/LzmaEnc.c`, `C/LzmaDec.c`, `C/XzDec.c`, `C/Xxh64.c`, `C/Alloc.c`, `C/Threads.c`.
- `CPP/7zip/Crypto/`: encryption/decryption support including 7z AES, ZIP crypto, WinZip AES, RAR AES, HMAC, PBKDF2, and random generation.
- `CPP/7zip/Common/`: codec registration, streams, buffers, temporary buffers, method property parsing, progress, and COM-style object helpers.
- `CPP/Common/`: foundational containers, strings, wildcard matching, command-line parsing, XML/text config helpers, hashing registration, and stream helpers.
- `CPP/Windows/`: Windows and portable-ish OS abstraction wrappers for files, directories, registry, DLL loading, processes, synchronization, shell, time, security utilities, and UI controls.

UI layer:

- Console: `CPP/7zip/UI/Console/`.
- GUI dialogs: `CPP/7zip/UI/GUI/`.
- Full file manager: `CPP/7zip/UI/FileManager/`.
- Explorer shell extension: `CPP/7zip/UI/Explorer/`.
- Far Manager plugin: `CPP/7zip/UI/Far/`.
- Shared UI operations: `CPP/7zip/UI/Common/`.

CLI layer:

- `CPP/7zip/UI/Common/ArchiveCommandLine.cpp` and `.h` define command types, switches, parsing state, archive selection, wildcard/listfile parsing, output stream routing, passwords, NTFS/links/streams options, and command-specific options.
- `CPP/7zip/UI/Console/Main.cpp` contains the help text and command execution routing.

Service/backend layer:

- There is no network service/backend layer. The backend is local archive processing via streams, filesystem APIs, codecs, and plugins.

Data/storage layer:

- Local filesystem and archive streams are the primary storage layers.
- Registry is used on Windows for 7-Zip installation/plugin path discovery and UI/settings integration, via files such as `CPP/Windows/Registry.cpp`, `CPP/7zip/UI/Common/LoadCodecs.cpp`, and `CPP/7zip/UI/FileManager/Registry*.cpp`.
- Temporary files/directories are used during updates, extraction, and self-extracting setup flows, for example `CPP/7zip/UI/Common/TempFiles.cpp`, `CPP/7zip/UI/Common/Update.cpp`, and `CPP/7zip/Bundles/SFXSetup/SfxSetup.cpp`.

Configuration system:

- Build-time configuration is makefile/macro driven. Examples: `Z7_EXTERNAL_CODECS`, `Z7_PROG_VARIANT_Z`, `Z7_EXTRACT_ONLY`, `DISABLE_RAR`, `DISABLE_RAR_COMPRESS`, `MY_DYNAMIC_LINK`, `PLATFORM`, `USE_ASM`, and compiler-specific makefile includes.
- Runtime configuration mostly comes from CLI switches, registry values, selected UI options, archive properties, and plugin discovery.

Plugin/extension system:

- Archive handlers and codecs register through static registration macros in `CPP/7zip/Common/RegisterArc.h` and `CPP/7zip/Common/RegisterCodec.h`.
- External codec/format loading is implemented in `CPP/7zip/UI/Common/LoadCodecs.cpp`, which looks near the executable and, on Windows, in registry paths such as `Software\7-zip\Path`, `Path32`, or `Path64`.
- DLL exports in `CPP/7zip/Archive/ArchiveExports.cpp` provide format enumeration and handler creation.

External integrations:

- Windows shell extension, registry, COM-like interfaces, DLL plugin loading, Windows GUI controls/resources, self-extracting setup execution, and optional Far Manager plugin integration.
- No direct internet/network integration was found in the inspected code paths.

## 5. Runtime Flow

For the console path:

1. `CPP/7zip/UI/Console/MainAr.cpp` enters `main`.
2. It binds `g_StdStream` and `g_ErrStream`, checks CPU ISA compatibility with `CPU_IsSupported_*`, runs `NT_CHECK`, installs a console control handler, and on Windows calls `My_SetDefaultDllDirectories()` from `C/DllSecur.c`.
3. It calls `Main2` in `CPP/7zip/UI/Console/Main.cpp`.
4. `Main2` handles console setup, command-line parsing through `CArcCmdLineParser` from `CPP/7zip/UI/Common/ArchiveCommandLine.cpp`, loads formats/codecs through `CCodecs`, applies switches and properties, and dispatches by command.
5. List/test/extract flows open archives through `CPP/7zip/UI/Common/OpenArchive.cpp`, call handlers implementing `IInArchive`, and route output through callbacks in `CPP/7zip/UI/Console/*CallbackConsole.*`.
6. Add/update/delete/rename flows use `CPP/7zip/UI/Common/Update.cpp`, `UpdateProduce.cpp`, and format-specific `IOutArchive` implementations where available.
7. Codec creation resolves registered codecs through `CPP/7zip/Common/CreateCoder.cpp`, static codec registration from `CPP/7zip/Compress/*Register.cpp`, and optional external codecs.
8. Format handlers read/write through `ISequentialInStream`, `ISequentialOutStream`, `IInStream`, and `IOutStream` abstractions from `CPP/7zip/IStream.h` and stream helpers in `CPP/7zip/Common/`.

For plugin/DLL paths:

1. A client loads a 7-Zip DLL module.
2. `CPP/7zip/Archive/DllExports.cpp` handles `DllMain`, exports `CreateObject`, `SetLargePageMode`, `SetCaseSensitive`, and `SetCodecs`.
3. `CPP/7zip/Archive/ArchiveExports.cpp` maps handler CLSIDs to registered `CArcInfo` records, returns handler metadata, and creates `IInArchive`/`IOutArchive` instances.

For GUI/file manager paths:

1. GUI/frontends enter their Windows application code in `CPP/7zip/UI/GUI/` or `CPP/7zip/UI/FileManager/`.
2. They reuse `UI/Common` extraction/update/open/hash code and the same archive/codecs layer as the console frontend.
3. Settings, context menu entries, passwords, dialogs, language resources, and system integration are handled in FileManager/GUI/Explorer-specific modules.

## 6. Build, Run, and Test Workflow

Install dependencies:

- Documented Windows dependency: Visual Studio compiler and/or Windows SDK, plus Microsoft Macro Assembler (`ml.exe` for x86, `ml64.exe` for x64), from `DOC/readme.txt`.
- Documented Unix-like dependencies: GCC or Clang; optional Asmc/UASM/JWasm for MASM-style x86/x64 assembly; GNU assembler for ARM64 assembly.
- No package manager install command is documented in the repo.

Build:

- Documented Windows full build:
  - `cd CPP\7zip`
  - initialize a Visual Studio environment such as `vcvars64.bat`
  - `nmake`
- Documented Windows single-binary build:
  - `cd CPP\7zip\Bundles\Alone`
  - `nmake`
- Documented Unix-like standalone all-format console build:
  - `cd CPP/7zip/Bundles/Alone2`
  - `make -j -f makefile.gcc`
- Documented Unix-like optimized/no-assembly alternatives:
  - `make -j -f ../../cmpl_gcc.mak`
  - `make -j -f ../../cmpl_clang.mak`
- Documented platform/feature knobs include `PLATFORM=x64|x86|arm64|arm|ia64`, `MY_DYNAMIC_LINK`, `USE_ASM=1`, `ST_MODE=1`, `DISABLE_RAR=1`, and `DISABLE_RAR_COMPRESS=1`.

Run:

- Inferred after build:
  - Console: run `7z.exe`, `7za.exe`, `7zz`, or `7zr.exe` depending on the built bundle.
  - File manager: run `7zFM.exe`.
  - GUI helper: run `7zG.exe`.
  - Shell extension: install/register the built shell extension through the project’s installer or Windows integration flow; exact manual registration steps are not fully documented in inspected files.

Run tests:

- No automated test suite was found.
- Inferred smoke tests after a successful build:
  - `7zz i` or `7z i` to list formats/codecs.
  - `7zz b` to run benchmark.
  - `7zz t archive.7z` to test an archive.
  - `7zz x archive.7z -o<dir>` to smoke-test extraction.
- `C/Util/7z/7zMain.c` is described by `DOC/7zC.txt` as a 7z ANSI-C decoder test application, but it is not a general project test suite.

Package/release:

- Windows installer metadata exists in `DOC/7zip.wxs`.
- SFX bundles exist under `CPP/7zip/Bundles/SFXCon/`, `SFXWin/`, and `SFXSetup/`.
- No release automation/CI pipeline was detected.

Validation note:

- I did not run builds because required compilers/build tools were not available on PATH in this shell.

## 7. File-by-File / Module-by-Module Dissection

### `README.md`

Minimal top-level readme. It only says "7-Zip on GitHub" and links to the upstream 7-Zip website. It does not document this fork’s purpose or divergence.

### `DOC/readme.txt`

Primary source package readme. It identifies the version, license overview, Windows and Unix build flows, RAR exclusion knobs, the distinction from p7zip, COM-module notes, and the source tree inventory.

### `DOC/License.txt`, `DOC/copying.txt`, `DOC/unRarLicense.txt`

License sources. `DOC/License.txt` gives the mixed license map. `DOC/copying.txt` contains LGPL text. `DOC/unRarLicense.txt` contains the RAR restriction that matters for forks and redistribution.

### `DOC/src-history.txt`

Release history. Important current signals include recent 26.01 changes, 25.x symbolic-link security CVEs, and 24.01 Zstandard support. This file is a useful changelog but not a fork-specific history.

### `DOC/7zFormat.txt`, `DOC/Methods.txt`, `DOC/lzma.txt`, `DOC/7zC.txt`

Format and method documentation. `DOC/Methods.txt` includes method IDs such as ZIP ZSTD (`5D`) and external codec ID ranges. `DOC/7zFormat.txt` documents the 7z structure. `DOC/7zC.txt` documents the C decoder/test app.

### `CPP/Build.mak`

Main Windows `nmake` rules. It sets compiler/linker flags, object output directory, warning level, static vs dynamic runtime, assembler selection, subsystem/stack flags, resource compilation, and clean target. It uses high warning levels and treats warnings as errors.

Important detail: `Build.mak` has a `clean` target that deletes binaries/objects in the output directory. That is destructive relative to build artifacts, so it should not be run casually if preserving local build outputs matters.

### `CPP/7zip/makefile`

Top-level Windows makefile for the 7-Zip C++ tree. It descends into `UI` and `Bundles` directories.

### `CPP/7zip/7zip.mak`

Shared Windows object compilation rules. It maps object groups such as `COMMON_OBJS`, `AR_OBJS`, `COMPRESS_OBJS`, `CRYPTO_OBJS`, `UI_COMMON_OBJS`, and `C_OBJS` to their source directories and includes `Asm.mak`.

### `CPP/7zip/7zip_gcc.mak`, `CPP/7zip/cmpl_*.mak`, `CPP/7zip/var_*.mak`, `CPP/7zip/warn_*.mak`

Unix-like/Mingw build plumbing. These define compiler flags, platform variables, assembly use, warning modes, and object build rules for GCC/Clang/macOS/ARM/x86 variants.

### `CPP/7zip/Bundles/Format7zF/Arc.mak`

Defines the "all formats" object set for Windows builds. It lists archive handlers, common archive helpers, compression codecs, crypto modules, and C codec engines. This is one of the best maps of what the full 7z format DLL/bundle contains.

### `CPP/7zip/Bundles/Format7zF/Arc_gcc.mak`

GCC counterpart to `Arc.mak`. Notable fork controls appear here: `DISABLE_RAR` excludes RAR handlers and forces `DISABLE_RAR_COMPRESS=1`; `DISABLE_RAR_COMPRESS` excludes RAR decompression codec objects.

### `CPP/7zip/Bundles/Alone2/makefile` and `makefile.gcc`

Build the standalone all-format console program `7zz.exe`/`7zz`. They include `Format7zF` and console objects, and define `Z7_PROG_VARIANT_Z`.

### `CPP/7zip/UI/Console/makefile` and `Console.mak`

Build the Windows console frontend `7z.exe` with `Z7_EXTERNAL_CODECS`, meaning it can load formats/codecs externally rather than embedding the all-format bundle the same way `7zz` does.

### `CPP/7zip/UI/Console/MainAr.cpp`

Outer console `main`. It initializes global streams, checks CPU instruction set support, applies Windows DLL search hardening via `My_SetDefaultDllDirectories()`, wraps execution in exception handling, and maps errors to 7-Zip exit codes.

Fragile/surprising behavior: CPU feature detection can fail before command parsing if the binary was compiled with instructions unsupported by the current CPU.

### `CPP/7zip/UI/Console/Main.cpp`

Main console implementation. It contains the visible help text, command handling, codec/format listing, benchmark handling, password propagation, extraction/test/list/update/hash paths, and console-specific callbacks.

Important details: supported commands include `a`, `b`, `d`, `e`, `h`, `i`, `l`, `rn`, `t`, `u`, and `x`. Sensitive switches include `-p`, `-sdel`, `-sfx`, `-si`, `-so`, `-snh`, `-snl`, `-sni`, `-sns`, `-spf`, and output directory/workdir switches.

### `CPP/7zip/UI/Common/ArchiveCommandLine.cpp` and `.h`

Defines command-line parsing, switch keys, `CArcCmdLineOptions`, archive command types, wildcard/listfile handling, extraction/update/hash options, output stream routing, recursion modes, path modes, link/security/alternate-stream options, and password state.

Security-relevant detail: the parser explicitly distinguishes symbolic link options, including `kSymLinks_AllowDangerous`, and NTFS security/alternate stream options.

### `CPP/7zip/UI/Common/OpenArchive.cpp`

Archive open orchestration. It selects candidate formats, opens files/streams, uses handler metadata and signatures, and manages nested archive links. I inspected related flow through callers, but did not exhaustively read every branch.

### `CPP/7zip/UI/Common/Extract.cpp`

Shared extraction workflow. It computes output directories, filters archive items, creates output directories, initializes `CArchiveExtractCallback`, prepares hard links where enabled, and calls `IInArchive::Extract`.

Important detail: output directory generation supports direct, add-archive-name, and replace-asterisk modes, reflecting the `-spo*` behavior described in history/help.

### `CPP/7zip/UI/Common/Update.cpp`

Shared update/archive creation workflow. It computes archive paths, temp paths, default archive types, SFX extension behavior, update commands, and error reporting. It contains Windows temp prefix `7zE` and handles multi-volume/update limitations.

Fragile/surprising behavior: update support depends on the selected handler having `IOutArchive`/update support; otherwise the user receives "update operations are not supported for this archive".

### `CPP/7zip/UI/Common/LoadCodecs.cpp`

Loads internal and external formats/codecs. Static handlers register via globals, and `Z7_EXTERNAL_CODECS` adds DLL loading. The plugin search order checks the executable directory, then Windows registry values under `Software\7-zip` such as `Path32`, `Path64`, and `Path`.

Security note: plugin DLL loading is intentionally part of the design. The path discovery logic makes DLL/plugin location a security-sensitive surface.

### `CPP/7zip/Common/RegisterArc.h`

Defines `CArcInfo` and registration macros such as `REGISTER_ARC_I`, `REGISTER_ARC_IO`, and `REGISTER_ARC_IO_DECREMENT_SIG`. Format handlers use these macros to statically register names, extensions, signatures, flags, and create functions.

### `CPP/7zip/Common/RegisterCodec.h`

Defines `CCodecInfo`, `CHasherInfo`, and registration macros for codecs, filters, and hashers. Codec modules use static constructors to populate global registries.

### `CPP/7zip/Common/CreateCoder.cpp`

Holds global codec/hasher arrays, implements `RegisterCodec`, `RegisterHasher`, external codec loading, method lookup by name or ID, and Coder creation helpers.

Constraint: registry arrays have fixed max sizes (`kNumCodecsMax = 64`, `kNumHashersMax = 32`), so adding many codecs/hashers may require capacity review.

### `CPP/7zip/Archive/IArchive.h`

Core archive interfaces. Defines handler flags, handler property IDs, extraction/update operation result enums, callback interfaces, and documentation on output ownership and threading assumptions.

Important interfaces: `IInArchive`, `IOutArchive`, `IArchiveOpenCallback`, `IArchiveExtractCallback`, and related callback/message interfaces.

### `CPP/7zip/ICoder.h`

Core compression interfaces. Defines `ICompressCoder`, `ICompressCoder2`, progress, coder properties, finish mode, memory limits, stream processed size reporting, and unused input buffer reading.

### `CPP/7zip/PropID.h`

Central property ID list used across archive handlers and UIs. Includes path, size, times, attributes, CRC, encryption, method, host OS, symlink/hardlink/alt-stream/NT security properties, warning/error flags, and user-defined property offset.

### `CPP/7zip/Archive/ArchiveExports.cpp`

Exports format enumeration and handler creation. It stores registered archive handlers, maps CLSIDs to archive IDs, exposes properties such as name, class ID, extensions, signatures, flags, and update support, and creates `IInArchive`/`IOutArchive` instances.

### `CPP/7zip/Archive/DllExports.cpp`

DLL entry and exports. It runs `NT_CHECK` in `DllMain`, exports `CreateObject`, `SetLargePageMode`, `SetCaseSensitive`, and optionally `SetCodecs`.

### `CPP/7zip/Archive/7z/`

Native 7z format implementation. Key files include:

- `7zHandler.cpp`: archive property reporting and method display.
- `7zIn.cpp` / `7zHeader.cpp`: read and parse 7z metadata.
- `7zDecode.cpp`: folder/coder decoding and password handoff.
- `7zExtract.cpp`: extraction callback coordination.
- `7zEncode.cpp`, `7zUpdate.cpp`, `7zHandlerOut.cpp`, `7zOut.cpp`: archive creation/update.
- `7zRegister.cpp`: format registration.

Important implementation detail: 7z compression method display resolves method IDs through the codec registry, so new codecs affect both processing and visible metadata.

### `CPP/7zip/Archive/Zip/`

ZIP implementation. `ZipHandler.cpp` recognizes method names including Deflate, Deflate64, BZip2, LZMA, zstd, xz, PPMd, LZFSE, and AES variants. `ZipIn.cpp`, `ZipOut.cpp`, `ZipUpdate.cpp`, and `ZipHandlerOut.cpp` cover parsing and update/write behavior.

Security-sensitive detail: ZIP supports multiple crypto schemes, including legacy ZipCrypto and strong crypto IDs; password handling flows through `IPassword` callbacks.

### `CPP/7zip/Archive/ZstdHandler.cpp`

Standalone `.zst` archive/stream handler. It parses Zstandard frame headers, exposes archive properties such as physical size, unpack size, stream/block counts, method, and checksum, and uses `NCompress::NZstd::CDecoder`.

Important detail: compression support is guarded by commented/disabled macros such as `Z7_USE_ZSTD_COMPRESSION`; the inspected code primarily supports ZSTD decompression unless those build-time macros and missing encoder files are supplied.

### `CPP/7zip/Compress/ZstdDecoder.cpp` and `.h`

C++ wrapper around `C/ZstdDec.c`. It implements `ICompressCoder`, buffer sizing properties, finish mode, unused input buffer recovery, decoder allocation, progress, output flushing, and error mapping from `SRes` to `HRESULT`.

Important detail: partial decoding is tolerated when finish mode is false; full stream validation depends on finish mode and known input/output sizes.

### `C/ZstdDec.c` and `C/ZstdDec.h`

Zstandard decoder engine. It is BSD 3-clause licensed according to `DOC/License.txt` and comments. It parses ZSTD frames, skip frames, checksums through XXH64, block types, FSE/Huffman/LZ-style sequences, memory windows, and decoder status.

Constraint: comments state the decoder limits window size to 2 GiB in the current configuration.

### `C/Xxh64.c` and `C/Xxh64.h`

XXH64 hashing used by ZSTD. BSD 2-clause according to `DOC/License.txt`.

### `C/Lzma*.c`, `C/Xz*.c`, `C/7z*.c`, `C/Ppmd*.c`, `C/Sha*.c`, `C/Aes*.c`

Core C codec and utility implementations inherited from 7-Zip/LZMA SDK style code. These provide the lower-level engines used by C++ wrappers and utilities.

### `C/Util/`

Utility programs and installer helpers:

- `C/Util/7z/`: ANSI-C decoder/test application.
- `C/Util/Lzma/`: LZMA utility.
- `C/Util/LzmaLib/`: LZMA library exports.
- `C/Util/7zipInstall/` and `7zipUninstall/`: Windows installer/uninstaller utilities.
- `C/Util/SfxSetup/`: C SFX setup support.

### `CPP/7zip/Crypto/`

Encryption/decryption support. Includes 7z AES, ZIP crypto, WinZip AES, RAR AES, HMAC-SHA1/SHA256, PBKDF2-HMAC-SHA1, random generation, and registration code.

Security note: legacy algorithms are present for archive compatibility. A fork should avoid implying that all supported archive crypto is modern or equally strong.

### `CPP/7zip/UI/FileManager/`

Windows file manager. Handles panels, folder browsing, archive opening, copying, drag/drop, passwords, properties, progress dialogs, registry settings, language/resource integration, alternate streams, and context menus.

### `CPP/7zip/UI/Explorer/`

Windows shell extension code. Handles context menu integration, registry context menu settings, Explorer command implementation, DLL exports, and resources.

### `CPP/Windows/`

Operating system abstraction and Windows wrappers. Security-relevant files include:

- `CPP/Windows/DLL.cpp`: DLL load wrappers.
- `CPP/Windows/FileDir.cpp`, `FileIO.cpp`, `FileFind.cpp`, `FileLink.cpp`: filesystem operations.
- `CPP/Windows/ProcessUtils.cpp`: `CreateProcess` wrappers.
- `CPP/Windows/Registry.cpp`: registry access.
- `CPP/Windows/SecurityUtils.cpp`: security descriptor utilities.

### `C/DllSecur.c` and `.h`

Windows DLL search hardening helper. `MainAr.cpp` calls `My_SetDefaultDllDirectories()` before the main console flow; this reduces unsafe DLL search behavior where supported.

### `Asm/`

Optimized assembly. `Asm/x86/` contains MASM-style optimized routines for AES, CRC, SHA, LZMA decode/find, sorting, and XZ CRC64. `Asm/arm64/` and `Asm/arm/` contain ARM-specific optimized routines. These improve performance but complicate portability and build setup.

## 8. Data Models and Interfaces

Important interfaces:

- `IInArchive` / `IOutArchive` in `CPP/7zip/Archive/IArchive.h`: archive open/read/extract/update contracts.
- `IArchiveExtractCallback` and related callbacks in `IArchive.h`: output stream acquisition, operation preparation, result reporting, progress.
- `ICompressCoder` / `ICompressCoder2` in `CPP/7zip/ICoder.h`: codec contracts.
- `ICompressSetCoderProperties`, `ICompressSetDecoderProperties2`, `ICompressSetFinishMode`, `ICompressSetMemLimit`: codec configuration contracts.
- `IPassword` interfaces in `CPP/7zip/IPassword.h`: password retrieval/set flows.

Important data structures:

- `CArcInfo` in `CPP/7zip/Common/RegisterArc.h`: archive format metadata, signature, extension, flags, create functions.
- `CCodecInfo` and `CHasherInfo` in `CPP/7zip/Common/RegisterCodec.h`: codec/hasher metadata and factory functions.
- `CArcCmdLineOptions` in `CPP/7zip/UI/Common/ArchiveCommandLine.h`: parsed CLI state, including command, paths, switches, streams, password, extraction/update/hash options, and NTFS/link/stream flags.
- `CZstdDecState`, `CZstdDecInfo`, and `CZstdDecResInfo` in `C/ZstdDec.h`: ZSTD decoder state, aggregate frame info, and decode result info.
- `CFrameHeader` in `CPP/7zip/Archive/ZstdHandler.cpp`: parsed ZSTD frame descriptor/window/dictionary/content-size fields.
- `CArchivePath`, `CUpdateOptions`, and update command structures in `CPP/7zip/UI/Common/Update.*`: archive naming and update state.

Config schemas and file formats:

- 7z format: documented in `DOC/7zFormat.txt`; implemented under `CPP/7zip/Archive/7z/` and `C/7z*.c`.
- Compression method IDs: documented in `DOC/Methods.txt`; used by codec registration and 7z/xz/zip handling.
- Build configuration: makefile variables and preprocessor macros, not a structured manifest.
- Windows resources: `*.rc`, `resource.h`, icons, bitmaps, and manifests under UI/bundle directories.
- WiX installer metadata: `DOC/7zip.wxs`.

CLI arguments:

- Command names are documented in `CPP/7zip/UI/Console/Main.cpp` help text and parsed through `ArchiveCommandLine.cpp`.
- Key commands: add, benchmark, delete, extract, hash, info, list, rename, test, update, extract-full-paths.
- Key switches include archive type (`-t`), method (`-m`), output dir (`-o`), password (`-p`), recursion (`-r`), listfile charset (`-scs`), console charset (`-scc`), hard/symbolic links (`-snh`, `-snl`), NT security (`-sni`), alternate streams (`-sns`), full paths (`-spf`), work dir (`-w`), SFX (`-sfx`), stdin/stdout (`-si`, `-so`), and delete-after-compress (`-sdel`).

Environment variables:

- No application-specific environment variable schema was found.
- Build tooling uses normal environment-provided compiler variables and platform variables. `Build.mak` references `NUMBER_OF_PROCESSORS`; Unix-like makefiles infer MinGW from `SystemDrive`/`SYSTEMDRIVE`.

Database schemas:

- None detected.

Network protocols:

- None detected in the inspected code.

## 9. Dependencies

Major in-repo dependencies:

- 7-Zip/LZMA SDK-style C and C++ code under `C/` and `CPP/`.
- ZSTD decoder implementation in `C/ZstdDec.c`, derived using Zstandard spec/original decoder as reference, BSD 3-clause.
- LZFSE decoder in `CPP/7zip/Compress/LzfseDecoder.cpp`, BSD 3-clause.
- XXH64 in `C/Xxh64.c`, BSD 2-clause.
- unRAR-derived RAR decompression code under `CPP/7zip/Compress/Rar*` and archive handling under `CPP/7zip/Archive/Rar/`, subject to unRAR restriction.

Native/system dependencies:

- Windows SDK, Visual Studio compiler, `nmake`, resource compiler, MASM for Windows builds.
- GCC/Clang and optionally assemblers such as Asmc/UASM/JWasm for Unix-like optimized builds.
- Windows libraries in `Build.mak` and UI makefiles: `oleaut32.lib`, `ole32.lib`, `user32.lib`, `advapi32.lib`, `shell32.lib`, and GUI-specific libraries such as `comctl32.lib`, `htmlhelp.lib`, `comdlg32.lib`, `gdi32.lib`.
- POSIX APIs on non-Windows builds, such as `unistd.h`, `sys/ioctl.h`, `sys/time.h`, and `sys/times.h` in `Main.cpp`.

Heavy dependencies:

- No third-party package-manager dependencies were found.
- The heavy dependency is the codebase itself: many bundled codecs, archive parsers, crypto implementations, UI layers, resource files, and assembly optimizations.

Security-sensitive dependencies:

- Crypto modules under `CPP/7zip/Crypto/`.
- DLL/plugin loading under `CPP/7zip/UI/Common/LoadCodecs.cpp` and `CPP/Windows/DLL.cpp`.
- Process launching in SFX/setup and process utilities.
- Archive parsers and decompression codecs, because they process untrusted input.

Abandoned/suspicious dependencies:

- No package-manager dependencies exist to classify as abandoned.
- Legacy Visual Studio `.dsp`/`.dsw` files are old but part of upstream 7-Zip compatibility support.
- RAR code is license-sensitive, not suspicious.

Dependencies that constrain a fork:

- LGPL obligations.
- unRAR restrictions if RAR decompression remains enabled.
- Windows-specific UI/shell integration if preserving full 7-Zip behavior.
- Assembly/toolchain requirements if preserving peak performance.
- Static registration and makefile object lists, which require manual updates when adding/removing formats/codecs.

## 10. Tests and Quality Signals

Existing quality signals:

- `CPP/Build.mak` uses high compiler warning levels and `-WX`/warnings-as-errors for MSVC. Clang paths enable `-Wall`, `-Wextra`, `-Weverything`, and `-Wfatal-errors`.
- GCC/Clang warning makefiles exist under `CPP/7zip/warn_*.mak` and `C/warn_*.mak`.
- The code contains extensive internal error propagation through `HRESULT`, `SRes`, `RINOK`, and explicit operation result enums.
- Release history in `DOC/src-history.txt` mentions security CVEs and bug fixes, showing upstream maintenance attention.
- Runtime archive testing exists as the `t` command and extraction test modes through `IArchiveExtractCallback`.

What appears untested:

- No CI configuration was found.
- No unit test framework or test data corpus was found.
- No scripted build matrix was found in-repo.
- GUI, Explorer shell extension, SFX behavior, plugin loading, symlink security behavior, and archive parser edge cases are not covered by visible automated tests in this checkout.

Local validation not performed:

- Build/test commands were not run because compilers and make tools were unavailable on PATH.

## 11. Security and Safety Notes

Concrete security-sensitive behavior:

- Filesystem access: archive extraction and update write local filesystem paths, create directories, preserve attributes/timestamps, optionally handle hard links, symbolic links, NT security descriptors, and alternate streams. Relevant files include `CPP/7zip/UI/Common/Extract.cpp`, `Update.cpp`, `CPP/7zip/UI/Common/ExtractingFilePath.cpp`, and `CPP/Windows/File*.cpp`.
- Symlinks/hardlinks: link support is explicit in properties and command switches (`-snl`, `-snh`). Release history documents 2025 symbolic-link CVE fixes and a switch `-snld20` for bypassing default checks when creating symbolic links.
- Alternate data streams: NTFS alternate stream support appears in properties, file manager code, and CLI switches (`-sns`).
- NT security descriptors: `-sni` and `kpidNtSecure`/`kNtSecure` support are present.
- Password handling: password strings flow through `IPassword` callbacks, console callbacks, file manager dialogs, and archive handlers. Some structures wipe passwords on destruction, for example `CCompressionMethodMode` in `CPP/7zip/Archive/7z/7zCompressionMode.h`.
- Crypto compatibility: legacy algorithms such as ZipCrypto, RC4 identifiers, DES/3DES-era ZIP strong crypto IDs, and RAR AES are present for compatibility. A fork should communicate crypto limitations clearly.
- DLL/plugin loading: `CPP/7zip/UI/Common/LoadCodecs.cpp` intentionally loads codec/format DLLs from executable/registered paths. `C/DllSecur.c` and `MainAr.cpp` show attention to Windows DLL search hardening through `SetDefaultDllDirectories` where available.
- Shell command/process execution: `CPP/Windows/ProcessUtils.cpp`, `CPP/7zip/Bundles/SFXSetup/SfxSetup.cpp`, and `C/Util/SfxSetup/SfxSetup.c` use `CreateProcess`/`ShellExecuteEx` to run extracted setup programs or helper processes.
- Deserialization/parsing: archive handlers parse many untrusted formats, including complex filesystems and containers. This is the largest attack surface.
- Unsafe language features: C/C++ manual memory management, pointer arithmetic, fixed-size arrays, and assembly are pervasive. The code often has explicit bounds/error checks, but memory safety remains a core risk class.

Plausible risks grounded in code:

- Plugin search/registry configuration can become a DLL loading risk if an attacker controls plugin paths or DLL contents.
- Archive extraction needs careful review before changing path normalization, symlink/hardlink handling, alternate streams, or output directory generation.
- Adding codecs/formats without updating registration limits and object lists can fail silently or produce incomplete builds.
- Running SFX setup modules executes extracted code by design; this is expected behavior but security-sensitive.

No network/TLS behavior was found, so TLS/HSTS/cipher-suite guidance does not appear applicable to this repository as inspected.

## 12. Forking Notes

Cleanly separable parts:

- `C/` codec engines and utilities can be used more independently than the UI layers, especially LZMA/XZ/ZSTD-related code.
- `CPP/7zip/Compress/` codec wrappers can be modified with less UI impact if interfaces remain stable.
- Individual archive handlers under `CPP/7zip/Archive/` are conceptually modular through `IArchive` and registration macros.
- Standalone bundle makefiles such as `CPP/7zip/Bundles/Alone2/` are good starting points for a CLI-only fork.

Tightly coupled parts:

- Build object lists in makefiles are manually coupled to source modules.
- Static registration relies on objects being linked; forgetting an object can silently remove a codec/format.
- UI/Common code ties command-line parsing, archive opening, extraction, update, callbacks, and codec loading together.
- Windows GUI/FileManager/Explorer layers depend on many `CPP/Windows/` wrappers and resources.

Easiest parts to replace:

- Top-level branding/documentation (`README.md`, release docs, displayed program strings) if license obligations are preserved.
- Build wrappers around existing makefiles.
- A CLI-only distribution based on `CPP/7zip/Bundles/Alone2`.
- Disabling RAR code in GCC builds through documented knobs.

Risky parts to modify:

- Path normalization, extraction output path generation, symlink/hardlink/alternate-stream behavior.
- Archive parsers and decompressors without a fuzz/test corpus.
- Crypto/password handling.
- DLL/plugin search and registry discovery.
- Assembly and CPU feature detection.
- Static registration macros and object-list makefiles.

Naming/branding/license issues:

- 7-Zip name, icons, and UI branding appear throughout paths, resources, program strings, docs, and installer metadata.
- LGPL and unRAR restrictions must be preserved and documented.
- If distributing modified RAR-related source, the unRAR restriction requires clear documentation/source comments that the code cannot be used to develop a RAR-compatible archiver.

Baked-in assumptions:

- 7-Zip COM-like interface style and GUID-based handler creation.
- Windows is the primary full-featured platform, even though CLI builds exist for Unix-like systems.
- Plugins/codecs are DLL/shared-library-like components discovered near the executable or through registry.
- Makefile/object-list based builds, not generated modern build systems.
- Archive formats and codecs are mostly enabled by linking source/object files, not by runtime module discovery alone.

## 13. Recommended First Changes

1. Add fork-specific documentation: expand `README.md` to explain what "WinZST" changes or intends to change relative to upstream 7-Zip.
2. Decide RAR policy early: keep RAR support with documented restrictions, or disable/remove it for license simplicity.
3. Create a reproducible build script for your target platform, preferably wrapping the existing makefiles rather than replacing them immediately.
4. Add CI for at least one supported build target once toolchain setup is known.
5. Add smoke tests around `7zz i`, `7zz b`, archive create/list/test/extract, `.zst` extraction, ZIP-with-ZSTD extraction, and failure cases for malformed archives.
6. Add path traversal/link extraction regression tests before changing extraction behavior.
7. Document plugin search paths and DLL loading behavior in fork docs.
8. Audit and rename branding/resources if distributing as a non-7-Zip product.
9. Keep initial modifications narrow: start with `CPP/7zip/Bundles/Alone2` for CLI-focused work or `CPP/7zip/Archive/ZstdHandler.cpp` / `CPP/7zip/Compress/ZstdDecoder.cpp` for ZSTD-focused changes.
10. Add a modern developer note mapping which makefile object lists must be edited when adding/removing a handler or codec.
11. Consider adding fuzzing later for archive parsers and codecs if the fork will process untrusted archives at scale.

## 14. Unknowns and Follow-Up Questions

- The exact purpose of this `WinZST` fork is not documented in the repository. The code appears close to upstream 7-Zip 26.01 with ZSTD support already present upstream.
- I could not verify whether the repository builds locally because no supported compiler/make tool was available on PATH.
- I did not compare this repository against official upstream 7-Zip source, so fork-specific diffs are unknown.
- I did not inspect every one of the 1,293 files line-by-line; I inspected top-level docs, build files, key entry points, registration systems, representative handlers/codecs, CLI flow, extraction/update/plugin/security-sensitive paths, and source inventory.
- It is unclear whether `WinZST` intends to add ZSTD compression, improve Windows packaging, provide a patched binary, or simply mirror 7-Zip releases.
- No CI/build artifacts in the repo show the maintainer’s expected compiler versions or packaging process beyond upstream docs.
- No vulnerability/fuzzing/test corpus is included, so parser robustness cannot be assessed from tests.

## Fork Readiness Verdict

**Forkable with cleanup**

The codebase is mature, recently updated, and self-contained, and the license terms are documented. However, the repository has almost no fork-specific documentation, no visible CI or test suite, no local build tooling available in this environment, and substantial licensing/security-sensitive surfaces around RAR code, archive extraction, DLL plugins, and C/C++ parser code. A fork is practical, but the first work should be documentation, reproducible builds, tests, and a clear scope decision.
