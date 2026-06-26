# AGENTS.md

# WinZST Engineering Specification

## 1. Project Thesis

WinZST exists to make tarballs worth downloading on Windows.

The core claim of this project is that compressed tar archives are already a good archive model, but Windows archive tooling often makes them feel awkward, indirect, or second-class. A `.tar.gz`, `.tgz`, `.tar.xz`, `.txz`, `.tar.zst`, or `.tzs` file should behave like a normal archive from the user's point of view. The user should not need to understand that tar and compression are technically separate layers. They should not extract a `.tar.gz` only to receive an intermediate `.tar` file. They wanted the contents. WinZST should give them the contents.

WinZST is a standalone Windows archive utility with broad archive support. It is not only a Zstandard decompressor. It is not only a tarball viewer. It is not a small demo wrapper around one codec. It is a general archive utility derived from the existing 7-Zip codebase, modified toward a different product opinion.

The product opinion is strict:

```text
Compressed tarball in -> final contents out
```

When the user extracts `example.tar.zst` and the archive contains `a.txt`, the expected result is `a.txt`, not `example.tar`.

That behavior is the center of the project.

## 2. Product Identity

WinZST is a Windows archive utility.

It should support common archive formats inherited from the existing archive engine, including ZIP, 7z, tar formats, compressed tar formats, and other formats already supported by the upstream-derived codebase where practical.

WinZST should not present itself as a cosmetic reskin of another project. It is based on existing 7-Zip source code and the README must state that clearly, but the public-facing identity should stand on its own. The program must not piggyback on 7-Zip's name, icon, branding, screenshots, reputation, or visual identity as a marketing crutch.

The correct public framing is:

```text
WinZST is a Windows archive utility focused on making tarballs behave correctly and making tar plus Zstandard a first-class archive workflow.
```

The incorrect public framing is:

```text
WinZST is 7-Zip but with a different icon.
```

That is not enough. The fork must earn its name through behavior.

## 3. Relationship to 7-Zip

This repository is derived from 7-Zip. That must be documented plainly in the README, license files, release notes, and source notices where appropriate.

The project should not hide its origin. Hiding the fork relationship is both dishonest and counterproductive. The existing codebase provides mature archive parsing, compression, extraction, GUI infrastructure, Windows integration, shell support, build rules, and format handling. That inheritance is useful.

However, WinZST must not use the 7-Zip name or image as its own brand.

Rules:

1. The README must mention that WinZST is derived from 7-Zip.
2. License obligations must be preserved.
3. Upstream copyright notices must remain intact.
4. The application name, icons, installer identity, file associations, release names, and user-facing branding must be WinZST.
5. Do not imply that WinZST is official 7-Zip.
6. Do not imply endorsement by upstream 7-Zip.
7. Do not use upstream branding to make the project seem more mature than it is.

The project should track upstream 7-Zip closely. Heavy divergence is discouraged. This fork should remain maintainable against upstream unless a specific WinZST requirement demands a controlled divergence.

The rule is:

```text
Track upstream by default. Diverge only for a clear WinZST behavior.
```

## 4. Primary Archive Model

WinZST is primarily tarball-first.

Zstandard-compressed tarballs are preferred for new archives, but the product is broader than Zstandard. The application should remain a general archive utility.

Preferred archive model for new archives:

```text
.tzs = .tar.zst
```

`.tzs` is the primary new archive format exposed by WinZST. It is the preferred default for new archives, especially when compressing folders or multiple files.

`.tzs` is not a proprietary format. It is not a new container. It is not a lock-in scheme.

A `.tzs` file must be a standard tar archive compressed with Zstandard.

The following must be true:

```text
archive.tzs == archive.tar.zst
```

A user should be able to rename `archive.tzs` to `archive.tar.zst` and use standard tar plus Zstandard tooling to extract it.

The value of `.tzs` is Windows-facing usability. It provides a shorter, cleaner, recognizable archive extension for a standard format that already exists.

## 5. Format Support Policy

WinZST should support many archive formats.

This project is not trying to become a narrow single-format tool. Since it inherits a broad archive engine, it should remain useful as a general archive utility.

The core supported creation formats should include:

```text
.tzs
.tar.zst
.tar.gz
.tgz
.tar.xz
.txz
.zip
.7z
```

Other inherited formats may remain supported where licensing, maintenance, security, and upstream tracking allow.

The archive creation UI should prefer `.tzs` for new archives while allowing the user to select expanded or legacy formats from a dropdown.

Expected archive creation dropdown behavior:

```text
WinZST archive (.tzs) [default]
Tarball with Zstandard (.tar.zst)
Tarball with gzip (.tar.gz)
Tarball with XZ (.tar.xz)
ZIP archive (.zip)
7-Zip archive (.7z)
Other supported formats, if applicable
```

`.7z` remains supported as a legacy and compatibility format. It should not be removed. However, WinZST should prefer Zstandard tarballs for new archive creation over LZMA2 `.7z` archives unless the user explicitly chooses `.7z`.

This does not mean `.7z` is bad. It means WinZST has a different default opinion.

## 6. Compressed Tarball Extraction Semantics

Compressed tarballs must extract like modern GNU tar extraction.

Given this input:

```text
example.tar.zst
```

And the archive contains:

```text
a.txt
```

The default extraction result must be:

```text
a.txt
```

The default extraction result must not be:

```text
example.tar
```

This applies to all supported compressed tarball variants:

```text
example.tar.gz
example.tgz
example.tar.xz
example.txz
example.tar.zst
example.tzs
example.tar.bz2
example.tbz2
```

The default user intent for a compressed tarball is full extraction of the contained files and folders.

The intermediate tar layer is an implementation detail. Do not expose it during normal extraction.

## 7. Decompress-Only Behavior

WinZST should support decompress-only behavior, but it must not be placed in the right-click quick actions.

Decompress-only means:

```text
example.tar.zst -> example.tar
example.tzs     -> example.tar
example.tgz     -> example.tar
```

This is valid advanced behavior for users who explicitly want the raw tar stream.

Rules:

1. Decompress-only must not be the default.
2. Decompress-only must not appear in the quick right-click menu.
3. Decompress-only may appear in the larger "Extract files..." dialog.
4. The UI must make clear that decompress-only produces the raw tar archive, not the final contents.
5. The normal "Extract here" and "Extract to folder" actions must fully extract compressed tarballs.

The right-click menu should stay focused on the common case.

## 8. Right-Click Menu Policy

The right-click menu should expose high-value actions only.

Preferred right-click actions:

```text
Open with WinZST
Extract here
Extract to folder
Compress to .tzs
Compress to .zip
Compress to .7z
```

Optional later actions:

```text
Test archive
Add to existing archive
Compress to...
```

Do not place advanced tar-layer operations in the quick menu.

Do not add confusing menu items that make users choose between tar and compression layers.

Do not expose "decompress only" from the right-click menu.

The right-click menu should reflect the thesis:

```text
Give the user the files, not the implementation detail.
```

## 9. GUI Direction

The first product target is the GUI.

WinZST inherits a full GUI stack from the existing codebase, so the project should use it rather than starting with a clean CLI-only prototype. The existing GUI, file manager, dialogs, resources, and shell integration are part of the project's practical advantage.

The initial goal is not to build a perfect application for random Windows users. The first real goal is:

```text
I can use this.
```

That is a valid v0.1 target.

However, "I can use this" does not mean unsafe, careless, or broken. It means the project may be rough, experimental, and unsigned, but it must still respect archive extraction safety and avoid regressions in inherited behavior.

The full GUI stack should be kept and modified. Do not remove the file manager, GUI dialogs, or inherited Windows UI structure just to make the codebase feel smaller. The product direction is a 7-Zip-like full archive utility, not a tiny toy app.

## 10. CLI Role

The CLI is not the first product target, but it remains important.

The CLI should be used for:

1. smoke testing,
2. regression testing,
3. scripted archive creation and extraction,
4. validating tarball semantics,
5. debugging handler behavior,
6. future automation.

The GUI is the main user-facing target, but the CLI is still a valuable validation surface.

If the GUI and CLI disagree on compressed tarball extraction semantics, that is a bug.

Both should follow the same core behavior:

```text
compressed tarball -> final contents
```

## 11. Existing Project Languages

WinZST will be written in the existing project languages.

Allowed implementation languages and file types include:

```text
C
C++
assembly where already used
Windows resource files
makefiles
installer metadata
plain-text documentation
```

Do not rewrite WinZST in Rust, Tauri, C#, Electron, Python, or another stack.

Do not introduce a new application framework to avoid understanding the existing code.

Do not start a parallel rewrite.

This project is an existing-codebase fork. The engineering path is to modify the current source tree carefully.

The current rule is:

```text
Work with the inherited codebase. Do not replace it.
```

A future rewrite can only be considered after the existing fork has proven its behavior, documented its requirements, and reached a stable working state. Until then, rewrite proposals are out of scope.

## 12. RAR Policy

WinZST may keep inherited RAR extraction support for user convenience.

RAR support is extraction-only. WinZST must not create `.rar` archives, must not advertise RAR creation, and must not use unRAR-derived code to recreate the proprietary RAR compression algorithm.

RAR is not a core WinZST feature. It exists only as inherited compatibility in a Windows archive utility.

All unRAR license notices and source restrictions must be preserved. If RAR code is modified or redistributed, documentation and source comments must continue to state the applicable unRAR restriction.

Do not place `.rar` in archive creation format lists.

Do include RAR extraction in regression testing if RAR support ships in a release.

## 13. Plugin Policy

External plugins should remain supported.

The inherited plugin system is useful and should not be removed without a specific reason. However, plugin loading is security-sensitive and must be documented.

Rules:

1. Keep plugin support.
2. Document plugin search paths.
3. Document whether plugins are loaded from the executable directory, registry-defined locations, or other configured paths.
4. Do not casually expand plugin search paths.
5. Do not load plugins from untrusted writable directories by default.
6. Do not make plugin behavior less predictable.
7. If plugin loading fails, report it clearly.
8. If a plugin changes archive support, make that visible to the user where practical.

Plugin support is allowed. Plugin sloppiness is not.

## 14. Upstream Tracking Policy

WinZST should track upstream 7-Zip closely.

This fork must not drift for no reason. Upstream has ongoing maintenance, bug fixes, security fixes, codec improvements, and format handling improvements. WinZST benefits from staying close.

Rules:

1. Keep upstream merges practical.
2. Keep WinZST-specific changes focused.
3. Avoid broad formatting churn.
4. Avoid unnecessary file moves.
5. Avoid replacing build systems early.
6. Avoid refactoring unrelated code.
7. Keep patches small enough to understand.
8. Document every intentional divergence from upstream behavior.
9. When modifying inherited code, prefer minimal targeted changes over sweeping rewrites.
10. If a change makes future upstream merges harder, it must provide clear WinZST value.

The project should maintain a patchset mindset.

The wrong approach:

```text
Rewrite everything because the code looks old.
```

The right approach:

```text
Change only what is needed to make WinZST behave correctly.
```

## 15. Extraction Safety Policy

Extraction safety beats compatibility.

Archive extraction processes untrusted input and writes to the filesystem. This is the most important safety boundary in the project.

Rules:

1. Do not weaken path traversal protections.
2. Do not silently extract absolute paths.
3. Do not silently write outside the selected destination.
4. Do not silently follow dangerous symlinks.
5. Do not silently create dangerous hardlinks.
6. Do not silently restore unsafe alternate streams.
7. Do not silently apply risky metadata.
8. Do not treat tar metadata as harmless.
9. Do not change extraction path normalization without tests.
10. Do not change symlink, hardlink, alternate stream, or security descriptor behavior casually.

If compatibility conflicts with safety, safety wins by default.

## 16. Dangerous Path Handling

If an archive contains dangerous paths, WinZST should show the dangerous path to the user and ask for permission.

Dangerous paths include, but are not limited to:

```text
absolute paths
paths containing ..
paths resolving outside the extraction directory
dangerous symlink targets
dangerous hardlink targets
paths requiring elevated locations
paths that overwrite protected files
paths involving risky alternate data streams
paths involving security descriptors that may not map safely
```

Default behavior should be to block or sanitize dangerous extraction.

If the user chooses to proceed, WinZST must show what is dangerous. Do not ask a vague question like:

```text
This archive contains dangerous paths. Continue?
```

Better:

```text
The archive wants to write outside the extraction folder:

Archive path:
../../AppData/file.txt

Resolved target:
C:\Users\Name\AppData\file.txt

This can overwrite files outside the chosen destination.
```

If administrator privileges are required, request elevation only after the user explicitly chooses to proceed. Do not elevate silently.

The user must understand what they are approving.

## 17. Windows-Safe Tar Metadata Policy

Windows-safe extraction is preferred over preserving all Unix tar metadata.

Tar archives can contain metadata that does not map cleanly to Windows, including:

```text
Unix permissions
owners
groups
symlinks
hardlinks
device files
special files
absolute paths
extended attributes
case-sensitive path conflicts
file modes
timestamps outside normal Windows ranges
```

WinZST should prioritize extracting useful files safely.

Rules:

1. Preserve normal files and directories first.
2. Preserve timestamps where safe and practical.
3. Preserve executable bits only if there is a clear Windows mapping or metadata display.
4. Do not fail the whole extraction because ownership cannot be restored.
5. Do not create Unix device files on Windows.
6. Treat symlinks and hardlinks as security-sensitive.
7. Treat absolute paths as dangerous.
8. Treat paths outside the selected destination as dangerous.
9. Prefer warnings and safe fallbacks over surprising writes.
10. Advanced options may expose more metadata behavior later, but safe defaults must remain.

The normal user wants the contents, not a perfect Unix filesystem reconstruction.

## 18. Archive Creation Defaults

WinZST should make good archive choices by default.

When a user selects files or folders and chooses a quick compress action, the default should be:

```text
.tzs
```

A `.tzs` archive must be standard tar plus Zstandard.

The archive creation dialog should allow choosing other formats:

```text
.tzs
.tar.zst
.tar.gz
.tgz
.tar.xz
.txz
.zip
.7z
```

The UI should explain `.tzs` plainly:

```text
WinZST archive (.tzs)
Standard tar archive compressed with Zstandard.
Compatible with tar plus zstd tools.
```

Do not describe `.tzs` as a new container. Do not imply it is proprietary. Do not hide the tar plus Zstandard structure.

Compression presets should be simple:

```text
Fast
Balanced
Maximum
Custom
```

Balanced should be the default unless later testing proves another default is better.

The normal archive creation path should not force users to choose raw codec details.

## 19. Archive Naming Rules

Archive names should be predictable.

When compressing a folder:

```text
FolderName -> FolderName.tzs
```

When compressing multiple selected files:

```text
Selected parent/context name -> Archive.tzs or prompted name
```

When compressing a single file:

```text
file.txt -> file.txt.tzs
```

or, if the UI has a better convention:

```text
file.txt -> file.tzs
```

The convention must be consistent.

When extracting:

```text
project.tzs -> project\
project.tar.zst -> project\
project.tar.gz -> project\
project.tgz -> project\
```

If the archive contains a single top-level folder, avoid unnecessary double nesting where practical.

If the archive contains loose files, create an output directory when the user chooses "Extract to folder" and extract directly when the user chooses "Extract here."

Do not surprise users with intermediate `.tar` files.

## 20. User-Facing Format Descriptions

Use plain names.

Good descriptions:

```text
WinZST archive (.tzs)
Tarball with Zstandard (.tar.zst)
Tarball with gzip (.tar.gz)
Tarball with XZ (.tar.xz)
ZIP archive (.zip)
7-Zip archive (.7z)
```

Bad descriptions:

```text
Next-generation archival container
Unix compression bundle
Multi-layer stream object
Zstandard transport envelope
```

Do not write nonsense marketing language.

Users should understand what they are creating.

## 21. Build Target

The first target platform is:

```text
Windows 11 x64
```

WinZST is not targeting Linux desktop builds, macOS builds, Windows on ARM, or 32-bit Windows as first-class targets at this stage.

Windows 10 support is not the main target. It may work if inherited code supports it, but Windows 11 x64 is the product target.

Rules:

1. Do not block early work on non-Windows targets.
2. Do not optimize early UI decisions around Windows 10.
3. Do not add Linux GUI requirements.
4. Do not spend early project time on macOS packaging.
5. Keep code portable where inherited code already is, but do not expand the scope.

The product is a Windows archive utility.

## 22. Release Policy

WinZST should eventually provide both:

```text
portable zip release
installer release
```

Both are useful.

Portable zip builds are good for early testing and self-use.

Installer builds are useful for file associations, Explorer integration, Start Menu entries, and normal Windows behavior.

Early builds may be unsigned from Windows' point of view. The project may use repository signatures or source/release integrity signatures, but early releases should not assume Microsoft-trusted code signing.

Rules:

1. Do not claim binaries are Microsoft-trusted if they are not.
2. Do not pretend SmartScreen warnings do not exist.
3. Do not block early releases on expensive code signing.
4. Do provide hashes or repo signatures where practical.
5. Do distinguish source authenticity from Windows Authenticode trust.
6. Do not overpromise install smoothness before signing exists.

Early releases can be rough, but they must be honest.

## 23. CI Policy

CI is not required before the first useful local build.

CI should come later.

When CI is added, the preferred direction is cross-compiling from Ubuntu if practical.

Rules:

1. Do not block initial feature work on CI.
2. Do not spend weeks designing CI before WinZST builds locally.
3. Add CI after the build process is understood.
4. CI should eventually build Windows x64 artifacts.
5. CI should eventually run smoke tests.
6. CI should eventually verify `.tzs` and compressed tarball behavior.
7. CI should eventually test path traversal and dangerous path cases.
8. If cross-compilation from Ubuntu proves impractical for some artifact type, document the limitation instead of pretending.

CI is important, but it is not the first milestone.

## 24. Testing Requirements

Even before full CI exists, behavior changes must be tested.

The minimum test targets are:

```text
open archive
list archive
create archive
test archive
extract archive
extract compressed tarball to contents
create .tzs
extract .tzs to contents
create .tar.zst
extract .tar.zst to contents
extract .tgz to contents
extract .tar.gz to contents
extract .txz to contents
extract .tar.xz to contents
```

Minimum safety tests:

```text
archive path with ..
absolute path
symlink outside destination
hardlink outside destination
overwrite attempt
alternate stream case
reserved Windows names
case-collision case
very long path
malformed tarball
truncated compressed stream
corrupt zstd stream
```

Minimum regression tests:

```text
.zip extraction still works
.7z extraction still works
.tar extraction still works
archive listing still works
archive testing still works
existing GUI extraction still works
existing GUI archive creation still works
```

A WinZST feature is not done if it works only for the happy path.

## 25. Regression Policy

WinZST must not introduce regressions in inherited archive behavior.

This is part of the failure definition.

The project fails if it cannot extract tarballs effectively. The project also fails if it breaks inherited 7-Zip behavior in formats users reasonably expect to keep working.

Rules:

1. Do not break ZIP.
2. Do not break 7z.
3. Do not break plain tar.
4. Do not break archive listing.
5. Do not break archive testing.
6. Do not break extraction destination handling.
7. Do not break password prompts.
8. Do not break plugin loading without a deliberate decision.
9. Do not break GUI workflows without replacing them with something better.
10. Do not remove inherited features casually.

WinZST can prefer `.tzs` without sabotaging other formats.

## 26. Documentation Requirements

The repository must clearly document what WinZST is.

Required documents:

```text
README.md
AGENTS.md
LICENSE or license notes
build notes
release notes when releases begin
```

README.md must state:

1. WinZST is a Windows archive utility.
2. WinZST is derived from 7-Zip.
3. WinZST focuses on sane tarball handling.
4. `.tzs` is standard `.tar.zst`.
5. `.tzs` is the preferred default for new archives.
6. The project enables RAR support by default.
7. Early builds may be experimental and unsigned.
8. The target platform is Windows 11 x64.

AGENTS.md must state engineering direction and project constraints.

Build notes must explain how to build the Windows x64 target.

Release notes must explain user-visible behavior changes and inherited upstream merges.

## 27. README Tone

The README should be direct.

Good README language:

```text
WinZST is a Windows archive utility derived from 7-Zip. It focuses on making compressed tarballs behave like normal archives on Windows. Its preferred archive format is .tzs, which is standard tar plus Zstandard compression.
```

Bad README language:

```text
WinZST revolutionizes the archival ecosystem with a next-generation compression platform.
```

Do not write like that.

Do not oversell.

Do not pretend the project is mature before it is.

Do not hide the fork origin.

Do not use upstream 7-Zip's brand as a substitute for WinZST's own identity.

## 28. UI Behavior Requirements

The GUI should behave like a full archive utility.

Expected GUI capabilities:

```text
open archives
browse archive contents
extract archives
create archives
test archives
show archive properties
prompt for passwords
show progress
allow cancellation
select output folder
select archive format
select compression level
```

Expected WinZST-specific GUI changes:

```text
prefer .tzs for new archives
show .tzs as standard tar plus Zstandard
extract compressed tarballs directly to contents
offer decompress-only only in advanced extract dialog
show dangerous paths before allowing risky extraction
avoid 7-Zip branding
```

The GUI should not become a radically different application in early versions.

Modify the inherited GUI. Do not replace it.

## 29. Shell Integration Requirements

Shell integration is important, but it should follow the core behavior.

Expected shell actions:

```text
Open with WinZST
Extract here
Extract to folder
Compress to .tzs
Compress to .zip
Compress to .7z
```

Shell extraction of compressed tarballs must fully extract contents by default.

Shell compression to `.tzs` must create a standard tar plus Zstandard archive.

Do not add "decompress only" to the quick shell menu.

Do not expose raw tar-layer implementation details in the common path.

Shell integration should be tested carefully because it touches Explorer, file associations, install state, and user expectations.

## 30. File Association Requirements

WinZST should associate with `.tzs`.

It may also associate with common archive formats depending on release maturity and user choice.

Required first-class associations:

```text
.tzs
.tar.zst
.tzst, if supported
.tzstd, if supported
.tar.gz
.tgz
.tar.xz
.txz
.tar
.zip
.7z
```

Association behavior should not hijack the system silently. The installer should be clear about what associations it applies.

Portable builds should avoid forcing associations unless the user explicitly runs a registration tool.

## 31. Installer Policy

Installer builds should support normal Windows expectations:

```text
install program files
register application
register file associations where selected
register shell menu entries
provide uninstall
preserve license notices
```

The installer must use WinZST branding.

The installer must not pretend to be 7-Zip.

The installer must not silently enable risky integration.

The installer should make file association choices clear.

The installer should not require Microsoft-trusted signing in early versions, but documentation must be honest about unsigned builds.

## 32. Portable Build Policy

Portable builds should be supported.

A portable release should be able to run without installation.

Portable builds may have limited shell integration unless the user explicitly registers it.

Portable builds should include:

```text
WinZST executable files
required DLLs
license files
readme
third-party notices
```

Portable builds are useful for early project use and testing.

## 33. License Policy

License compliance is mandatory.

Rules:

1. Preserve upstream copyright notices.
2. Preserve LGPL notices where required.
3. Preserve third-party notices.
4. Document enabled RAR support.
5. Do not remove license files to make the project look simpler.
6. Do not mix new code under incompatible terms.
7. Do not claim ownership of inherited code.
8. Do not use upstream trademarks as WinZST branding.
9. Document significant source modifications.
10. Keep source availability obligations in mind for binary releases.

License sloppiness is not acceptable.

## 34. Security-Sensitive Areas

The following areas require extra care:

```text
archive extraction paths
path normalization
symlink handling
hardlink handling
alternate data streams
NT security descriptors
file overwrite behavior
password handling
crypto code
archive parsers
decompression codecs
plugin loading
DLL loading
SFX behavior
process launching
installer behavior
shell extension behavior
```

Changes in these areas must be narrow, reviewed carefully, and tested.

Do not casually refactor these areas for style.

Do not make broad changes without understanding the original behavior.

Do not change security defaults to make one archive extract more conveniently.

## 35. Path Handling Requirements

Path handling is critical.

Rules:

1. Normalize archive paths before extraction.
2. Detect paths that escape the destination.
3. Detect absolute paths.
4. Detect drive-qualified Windows paths.
5. Detect UNC paths.
6. Detect `..` traversal.
7. Detect dangerous symlink and hardlink targets.
8. Detect reserved device names where applicable.
9. Detect confusing path collisions where practical.
10. Show dangerous resolved paths to the user before allowing risky extraction.

Default behavior must be safe.

Permission to extract dangerous paths must be explicit.

## 36. Symlink and Hardlink Requirements

Symlinks and hardlinks are dangerous when extracting untrusted archives.

Default behavior should prioritize safety over perfect preservation.

Rules:

1. Do not create symlinks that escape the extraction destination without explicit permission.
2. Do not create hardlinks that escape the extraction destination without explicit permission.
3. Show link source and target when asking for permission.
4. Require elevation if Windows requires it.
5. Do not silently replace normal files with links.
6. Do not silently follow archive-provided links during extraction in a way that writes outside the destination.
7. Prefer extracting link metadata as text or skipping with warning over unsafe creation.
8. Keep behavior consistent between GUI and CLI.

Symlink compatibility is less important than not writing files where the user did not intend.

## 37. Overwrite Policy

WinZST must not overwrite files silently unless the user has clearly chosen that behavior.

Default extraction should prompt, skip, rename, or use existing inherited safe behavior.

The UI should make overwrite choices clear:

```text
overwrite
skip
rename
apply to all
cancel
```

Dangerous overwrites, such as overwriting outside the destination or protected locations, require stronger warning and possibly elevation.

Do not treat overwrite prompts as a nuisance to remove.

## 38. Password and Encryption Policy

WinZST may inherit password and encryption support from the existing archive engine.

Rules:

1. Do not weaken password handling.
2. Do not log passwords.
3. Do not display passwords unless the user chooses show-password behavior.
4. Do not imply all archive encryption methods are equally strong.
5. Do not introduce new crypto.
6. Do not modify crypto code unless necessary.
7. Do not make `.tzs` encryption claims unless encryption is actually implemented and tested.

Tar plus Zstandard is not inherently encrypted.

If encrypted archives are supported through inherited formats such as `.7z` or `.zip`, document behavior accurately.

## 39. `.tzs` Compatibility Requirements

A `.tzs` file must be compatible with standard tools.

Required truth:

```text
.tzs is .tar.zst
```

A `.tzs` archive should be extractable by standard tooling after renaming to `.tar.zst`, assuming the tooling supports tar and Zstandard.

WinZST must not add private headers, private metadata, hidden manifests, or custom wrappers that make `.tzs` incompatible with normal tar plus Zstandard tools.

Any WinZST-specific metadata must be avoided unless it can be stored in a standards-compatible, harmless way. Even then, it should not be required for extraction.

## 40. `.tzs` Creation Requirements

Creating `.tzs` means:

1. collect selected files and folders,
2. create a tar archive stream,
3. compress that stream with Zstandard,
4. write the result with `.tzs` extension.

The output must not be a `.7z` archive renamed to `.tzs`.

The output must not be a ZIP archive renamed to `.tzs`.

The output must not be a custom WinZST container.

The output must be tar plus Zstandard.

## 41. `.tzs` Extraction Requirements

Extracting `.tzs` means:

1. detect `.tzs`,
2. treat it as tar plus Zstandard,
3. decompress the Zstandard layer,
4. read the tar layer,
5. extract final contents to the selected destination,
6. avoid producing an intermediate `.tar` by default.

The user should see archive contents, not only the raw tar stream.

If browsing `.tzs` in the GUI, the archive view should show the contents inside the tar archive.

## 42. Expanded `.tar.zst` Policy

WinZST should support `.tar.zst` as a first-class format.

`.tzs` is the preferred short Windows-facing extension, but `.tar.zst` is the explicit expanded name.

Users should be able to choose `.tar.zst` from the archive format dropdown.

Extraction behavior for `.tar.zst` and `.tzs` should be equivalent.

Creation behavior should differ only in filename extension.

## 43. gzip and XZ Tarball Policy

WinZST should support `.tar.gz`, `.tgz`, `.tar.xz`, and `.txz`.

These are not the flagship new format, but they are essential for tarball utility on Windows.

Extraction behavior must match `.tzs` semantics:

```text
compressed tarball -> final contents
```

Creation support should be available from the archive format dropdown.

Do not make users create `.tar` first and then compress it manually.

## 44. ZIP and 7z Policy

ZIP and 7z remain supported.

ZIP is necessary for Windows compatibility.

7z is necessary for inherited utility and existing archive compatibility.

WinZST may prefer `.tzs`, but it must not treat ZIP or 7z as enemies.

Expected behavior:

1. ZIP extraction works.
2. ZIP creation works.
3. 7z extraction works.
4. 7z creation works.
5. Existing archive browsing behavior remains functional.
6. Password behavior remains functional where inherited.
7. Testing archives remains functional.

Do not regress common formats while improving tarballs.

## 45. Failure Definition

WinZST fails if either of the following is true:

```text
It cannot extract tarballs effectively.
```

or:

```text
It introduces regressions in inherited 7-Zip behavior.
```

This is the project failure bar.

A build that merely compiles is not enough.

A build that has the WinZST name but still extracts `.tar.gz` into `.tar` by default is not enough.

A build that handles `.tzs` but breaks normal ZIP and 7z workflows is not enough.

A build that is pretty but unsafe is not enough.

## 46. Success Definition

WinZST succeeds when it makes tarballs worth downloading on Windows.

A successful early version allows the maintainer to:

```text
install or run WinZST on Windows 11 x64
open common archives
create .tzs archives
extract .tzs archives directly to contents
extract .tar.zst directly to contents
extract .tgz and .tar.gz directly to contents
extract .txz and .tar.xz directly to contents
still use ZIP and 7z normally
avoid unsafe extraction by default
```

That is enough for the early project to matter.

## 47. Version 0.1 Target

The v0.1 target is personal usability.

The target user is the maintainer.

v0.1 does not need to satisfy random Windows users. It does not need to be polished. It does not need Microsoft-trusted signing. It does not need perfect CI. It does not need a redesigned interface.

v0.1 must satisfy:

```text
I can use this for tarballs on Windows.
```

Minimum v0.1 requirements:

```text
Windows 11 x64 build
WinZST branding started
README explains the fork
AGENTS.md present
RAR enabled in default build
GUI launches
common archive opening works
.tzs is recognized
.tzs means .tar.zst
compressed tarballs extract to contents by default
.zip and .7z do not obviously regress
portable zip release can be produced
installer path is understood or partially working
```

## 48. Version 0.2 Target

The v0.2 target is daily utility.

Expected improvements:

```text
better .tzs creation
better archive format dropdown
better extract dialog wording
decompress-only available in advanced extract dialog
dangerous path warning UI improved
portable zip release repeatable
installer release repeatable
file associations improved
shell menu behavior improved
basic smoke tests documented
```

## 49. Version 0.3 Target

The v0.3 target is reduced embarrassment.

Expected improvements:

```text
more complete branding cleanup
fewer inherited 7-Zip visual leftovers
better README
better release notes
better build documentation
better regression checks
more predictable installer
more predictable portable build
clearer plugin documentation
```

## 50. Build System Policy

Do not replace the build system early.

The existing makefile-based build system is part of the inherited codebase. It may be old, but replacing it before the product behavior works is a bad trade.

Rules:

1. Use the inherited build system initially.
2. Add wrapper scripts if helpful.
3. Document the build commands.
4. Do not introduce CMake, Meson, Bazel, Cargo, npm, or other build systems early.
5. Do not churn object lists without understanding static registration.
6. When adding or removing archive handlers/codecs, update the required makefile object lists.
7. Keep Windows 11 x64 as the target.

Modern build work can come later.

## 51. Source Layout Policy

Do not reorganize the source tree early.

The inherited source layout is tied to build files, static registration, resources, and platform assumptions.

Rules:

1. Avoid moving source files.
2. Avoid renaming inherited directories.
3. Avoid mass formatting.
4. Avoid style-only refactors.
5. Keep WinZST-specific documentation at the top level.
6. Keep branding changes targeted.
7. Keep behavior changes near the relevant handler/UI code.

The source tree can be ugly. A working maintainable fork is more important than aesthetic restructuring.

## 52. Static Registration Policy

Archive formats and codecs may be registered through static objects and manual object lists.

When adding `.tzs` behavior or modifying tar plus Zstandard behavior, verify:

```text
handler metadata
extension lists
archive type names
codec availability
object lists
GUI format dropdowns
CLI format switches
DLL/plugin exports
file associations
installer associations
```

Do not assume adding a source file is enough.

Do not assume changing one extension string updates all UI, shell, and creation behavior.

## 53. Zstandard Policy

Zstandard is preferred for new WinZST tarballs.

Rules:

1. `.tzs` uses Zstandard.
2. `.tar.zst` uses Zstandard.
3. Zstandard support must be tested for extraction.
4. Zstandard support must be tested for creation before `.tzs` creation is considered working.
5. Do not fake `.tzs` creation by producing another format.
6. If inherited code has only partial Zstandard support in a path, document it and fix the correct layer.
7. Do not break ZIP-with-Zstandard handling if inherited support exists.

Zstandard is important, but tarball semantics are the actual product.

## 54. Performance Policy

WinZST should be reasonably fast, but correctness comes first.

Do not make unsafe changes for speed.

Do not remove existing optimized code without a reason.

Do not rewrite codecs for aesthetics.

Compression presets should balance speed and size:

```text
Fast
Balanced
Maximum
Custom
```

The default should be practical, not extreme.

For most users, a good `.tzs` default should compress quickly enough to feel modern and reduce size enough to justify the archive.

## 55. Error Handling Policy

Error messages should be clear.

Bad:

```text
Operation failed.
```

Better:

```text
WinZST could not extract this archive because one path tries to write outside the selected folder.
```

Errors should distinguish:

```text
unsupported format
corrupt archive
wrong password
dangerous path blocked
write permission denied
disk full
file already exists
plugin load failure
codec missing
feature disabled
```

Do not hide important details.

Do not expose useless internal noise to normal users unless advanced details are requested.

## 56. Dangerous Archive Dialog Policy

When an archive contains dangerous paths, the dialog must show specific information.

Required information where available:

```text
archive path
resolved output path
reason it is dangerous
default action
available choices
whether elevation is required
```

Allowed choices may include:

```text
skip dangerous entry
skip all dangerous entries
extract anyway
cancel extraction
```

"Extract anyway" should require explicit user selection.

If elevation is required, ask only after the user chooses the dangerous action.

## 57. No Update Warning Requirement

WinZST does not need a visible warning about updates yet.

Do not add a generic scary warning such as:

```text
Archive tools process untrusted input and must be updated.
```

That may be true in a broad sense, but it is not part of the current product requirement.

Focus on actual extraction safety behavior rather than generic warning banners.

Security should be implemented, not merely announced.

## 58. Branding Cleanup Policy

Branding cleanup is required before public releases.

Areas to audit:

```text
application name
window titles
about dialog
icons
installer name
file descriptions
version resources
Start Menu entries
Explorer context menu labels
file association labels
documentation
readme
release archive names
binary names if practical
```

Do not ship a confusing hybrid identity where the user cannot tell whether they are using WinZST or 7-Zip.

The README can and should mention the fork origin. The application UI should be WinZST.

## 59. Inherited Help Text Policy

Help text and dialogs inherited from upstream should be reviewed.

Rules:

1. Rename user-facing product references.
2. Add `.tzs` explanation where appropriate.
3. Update format lists where appropriate.
4. Redacted
5. Avoid over-documenting internals in normal dialogs.
6. Keep advanced documentation available for power users.

Help text should reflect actual behavior.

## 60. File Manager Policy

The full file manager should remain.

Do not remove it.

Do not replace it with a minimal interface.

The file manager should be modified to support WinZST's product behavior:

```text
show .tzs archives correctly
browse compressed tarball contents correctly
create .tzs archives
extract compressed tarballs directly
show clear archive properties
avoid upstream branding
```

The existing file manager is useful because users expect an archive utility to browse archives.

## 61. Archive Browse Policy

When opening a compressed tarball in the GUI, the user should see final archive contents.

For example:

```text
project.tar.gz
```

should display:

```text
project/
README.md
src/
```

or whatever the tar archive contains.

It should not merely display:

```text
project.tar
```

unless the user explicitly chooses raw stream/decompress-only mode.

Archive browsing should follow the same model as extraction.

## 62. Archive Test Policy

Archive testing should test the full compressed tarball path.

Testing `project.tar.zst` should validate both:

```text
Zstandard layer
tar archive layer
```

Testing `project.tzs` should do the same.

Do not report a compressed tarball as successfully tested if only the outer compression stream was tested and the tar archive inside was ignored.

## 63. Archive Property Policy

Archive properties should be honest about layers without making normal users manage layers.

For `.tzs`, properties may show:

```text
Type: WinZST archive (.tzs)
Structure: tar + Zstandard
Compression: Zstandard
Archive layer: tar
```

This is useful.

But normal extraction UI should not force the user to act on these layers manually.

## 64. Format Dropdown Policy

The archive creation dropdown must make `.tzs` the default.

Suggested order:

```text
WinZST archive (.tzs)
Tarball with Zstandard (.tar.zst)
ZIP archive (.zip)
7-Zip archive (.7z)
Tarball with gzip (.tar.gz)
Tarball with XZ (.tar.xz)
Other supported formats
```

The exact order may change after use, but `.tzs` should remain the preferred default.

Do not bury `.tzs`.

Do not make `.7z` the default unless the user selected it previously and the UI intentionally remembers that preference.

## 65. Settings Policy

If the GUI remembers archive creation settings, it must not accidentally erase the WinZST default.

User-selected settings may be remembered, but the default project recommendation remains `.tzs`.

If a user chooses `.zip`, that choice can be remembered if inherited behavior already does that. However, first-run behavior should prefer `.tzs`.

## 66. First-Run Behavior

First-run behavior should make the project opinion visible without being annoying.

Preferred:

```text
Default archive format: WinZST archive (.tzs)
Description: standard tar plus Zstandard
```

Avoid modal onboarding popups unless necessary.

Do not lecture the user about tarballs.

Make the default useful.

## 67. Admin and Elevation Policy

WinZST should not require administrator privileges for normal use.

Normal archive extraction and creation should work without elevation.

Elevation may be required for:

```text
installing shell integration
writing to protected directories
extracting dangerous paths into protected locations
registering machine-wide file associations
```

Do not elevate automatically.

Ask clearly.

If a dangerous archive path requires elevation, show the path and ask for permission first, then request elevation only if the user proceeds.

## 68. Scope Control

The project scope is broad archive utility behavior with tarball-first defaults.

In scope:

```text
GUI archive utility
compressed tarball handling
.tzs support
.tar.zst support
ZIP support
7z support
archive creation
archive extraction
archive testing
archive browsing
file associations
Explorer integration
portable releases
installer releases
RAR enabled builds
plugin support
Windows 11 x64 target
```

Out of scope for early work:

```text
Rust rewrite
Tauri rewrite
cloud storage integration
accounts
telemetry
custom archive container
new encryption format
mobile app
Linux GUI target
macOS target
Windows ARM target
full build-system replacement
large source tree reorganization
```

## 69. Telemetry Policy

Do not add telemetry.

WinZST does not need accounts, analytics, background services, tracking, or usage reporting.

Archive utilities should be boring and local.

If crash reporting is ever considered, it must be opt-in and documented. That is not an early goal.

## 70. Network Policy

WinZST should not require network access for core archive functionality.

Opening, creating, extracting, and testing archives should be local operations.

Do not add online dependencies to normal use.

Do not make archive extraction depend on a service.

Do not add auto-download behavior for codecs or plugins without a very strong reason and explicit user consent.

## 71. SFX Policy

Self-extracting archive support is inherited and security-sensitive.

Do not improve or expand SFX behavior early.

Do not make SFX a selling point.

If inherited SFX code remains, ensure branding and licensing are accurate.

If SFX behavior executes extracted programs, treat that path as security-sensitive.

## 72. Dependency Policy

Avoid new dependencies.

The inherited codebase is already large. Adding dependencies increases build complexity, security surface, and maintenance work.

Rules:

1. Do not add dependencies for simple UI text changes.
2. Do not add dependencies for archive behavior already possible in inherited code.
3. Do not add package managers early.
4. Do not add framework dependencies.
5. Do not add compression libraries unless needed and license-compatible.
6. If a dependency is added, document why it is required.

Prefer using the existing archive engine.

## 73. Code Style Policy

Follow the surrounding code style.

Do not reformat whole files.

Do not apply modern style preferences across inherited source files.

Do not rename variables or restructure functions unless needed for the behavior change.

The goal is to keep upstream merging practical.

Small targeted patches are better than large aesthetic changes.

## 74. Commit Policy

Commits should be focused.

Good commits:

```text
Document WinZST thesis
Enable RAR in default build
Add .tzs extension mapping
Make .tar.zst open as tarball contents
Add .tzs creation option
Rename GUI product strings
Add dangerous path warning detail
```

Bad commits:

```text
Mass reformat source tree
Rewrite UI framework
Move every file
Change unrelated archive handlers
Rename everything before behavior works
```

Keep changes reviewable.

## 75. Release Naming Policy

Release files should use WinZST names.

Examples:

```text
WinZST-v0.1.0-win11-x64-portable.zip
WinZST-v0.1.0-win11-x64-installer.exe
```

Do not release binaries named as though they are official 7-Zip.

If inherited binary names remain during early development, mark them as development artifacts and do not present them as final WinZST releases.

## 76. Repository Signature Policy

The project may use repository signatures and release integrity signatures.

Early builds do not need Microsoft-trusted Authenticode signatures.

Documentation should distinguish:

```text
repository/source signature
release artifact checksum
Microsoft-trusted code signing
```

Do not conflate them.

If a release is unsigned for Windows trust purposes, say so.

## 77. Compatibility Policy

WinZST should preserve compatibility where it does not conflict with project goals or safety.

Compatibility priorities:

1. safe extraction,
2. `.tzs` as `.tar.zst`,
3. compressed tarball contents extraction,
4. ZIP and 7z regression avoidance,
5. broad inherited archive support,
6. upstream tracking.

Compatibility is not an excuse to preserve bad compressed tarball UX.

If old behavior extracts `.tar.gz` to `.tar` by default, WinZST should change that.

## 78. User Prompt Policy

Prompt users when the action is risky or ambiguous.

Do not prompt for every normal archive operation.

Prompt for:

```text
dangerous paths
overwrite choices
elevation
decompress-only advanced behavior
unsupported metadata decisions where data loss may surprise the user
```

Do not prompt for:

```text
normal .tzs extraction
normal .tar.gz extraction
normal archive browsing
normal .tzs creation
```

Good defaults are better than endless questions.

## 79. Advanced Mode Policy

Advanced options are allowed, but they must not pollute the common path.

Advanced options may include:

```text
decompress only
preserve extra metadata
raw stream extraction
custom compression level
custom Zstandard settings
solid/non-solid choices where relevant
path mode details
link handling behavior
```

The common path should remain simple.

Most users should not need to understand tar layers, codec windows, dictionary sizes, or Unix metadata to extract files.

## 80. Archive Layer Model

Internally, WinZST must respect archive layers.

Externally, it should present compressed tarballs as single archive objects.

Internal model:

```text
.tzs -> zstd stream -> tar archive -> files
.tar.zst -> zstd stream -> tar archive -> files
.tgz -> gzip stream -> tar archive -> files
.txz -> xz stream -> tar archive -> files
```

User model:

```text
archive -> files
```

Do not confuse the user with the internal model unless they ask for advanced behavior.

## 81. Documentation Examples

Use concrete examples in docs.

Good example:

```text
If example.tzs contains a.txt, extracting example.tzs produces a.txt. It does not produce example.tar.
```

Good example:

```text
A .tzs file is the same structure as .tar.zst. The shorter extension exists so it feels like a normal Windows archive file.
```

Bad example:

```text
The project introduces an advanced archive abstraction layer.
```

The docs should explain behavior, not posture.

## 82. Maintainer Reality

This project does not need to be perfect on the first run.

The first audience is the maintainer.

The first useful release can be rough.

However, archive tools touch user data. Rough does not mean reckless.

Acceptable early roughness:

```text
ugly dialogs
incomplete branding cleanup
manual build steps
unsigned builds
limited installer polish
missing CI
unfinished website
```

Unacceptable early roughness:

```text
unsafe extraction defaults
fake .tzs files
silent dangerous path writes
broken ZIP extraction
broken 7z extraction
unclear license status
pretending to be official 7-Zip
```

Move fast where mistakes are cheap. Move carefully where mistakes can destroy files.

## 83. What Not To Optimize Yet

Do not optimize early for:

```text
mass adoption
perfect installer UX
Microsoft-trusted signing
store distribution
enterprise deployment
cross-platform GUI
large contributor base
plugin ecosystem
visual redesign
complete documentation site
```

Optimize early for:

```text
correct tarball extraction
.tzs creation and extraction
GUI usability for the maintainer
safe extraction behavior
not breaking inherited formats
clear fork documentation
```

## 84. What To Build First

The first work should be:

1. Add README that states WinZST's identity.
2. Add this AGENTS.md.
3. Confirm Windows 11 x64 GUI build.
4. Start branding cleanup enough to distinguish WinZST.
5. Enable RAR in the default build.
6. Verify inherited ZIP, 7z, and tar extraction still work.
7. Add `.tzs` extension recognition as `.tar.zst`.
8. Make `.tzs` open to tar contents.
9. Make `.tar.zst` open to tar contents.
10. Make `.tgz` and `.tar.gz` open to tar contents.
11. Make `.txz` and `.tar.xz` open to tar contents.
12. Add `.tzs` creation.
13. Add `.tzs` as preferred archive format in GUI.
14. Add advanced decompress-only option outside the right-click menu.
15. Add dangerous path confirmation behavior if current inherited behavior is not sufficient.

Do not start with a full UI redesign.

## 85. What To Avoid First

Avoid these early:

```text
rewriting the app
replacing the build system
adding dependencies
adding telemetry
adding cloud features
adding accounts
supporting non-Windows targets
polishing every icon
rewriting every dialog
refactoring archive parsers
changing crypto
expanding SFX support
changing plugin loading broadly
```

These can derail the project before the core archive behavior works.

## 86. Maintenance Policy

Maintenance should prioritize:

1. upstream 7-Zip security and bugfix merges,
2. tarball behavior correctness,
3. `.tzs` compatibility,
4. Windows 11 x64 builds,
5. regression avoidance,
6. release reproducibility,
7. documentation clarity.

Do not let cosmetic changes outrank archive behavior.

Do not let new features outrank extraction safety.

## 87. Open Questions

The following may be decided later:

```text
whether Windows 10 becomes officially supported
whether Linux CLI builds are published
whether Windows ARM builds are published
whether portable builds can register optional shell integration
whether .tzs should get a custom icon distinct from .tar.zst
whether plugins should be shown in a GUI management screen
whether file association selection is per-user or machine-wide
whether CI cross-compilation from Ubuntu can build every artifact
whether future signing becomes affordable or necessary
```

Do not block current work on these questions.

## 88. Final Engineering Rule

Every meaningful change should be checked against these questions:

1. Does this make tarballs better on Windows?
2. Does this preserve `.tzs` as standard `.tar.zst`?
3. Does this avoid exposing intermediate `.tar` files by default?
4. Does this keep extraction safe?
5. Does this avoid regressions in inherited archive behavior?
6. Does this keep upstream tracking practical?
7. Does this avoid unnecessary rewrites?
8. Does this fit Windows 11 x64 as the target?
9. Does this keep WinZST honest about being a fork?
10. Does this make the program more useful to the maintainer?

If the answer is no to most of these, the change probably does not belong yet.

## 89. Final Claim

WinZST is not trying to invent the future of compression.

WinZST is trying to make archive formats people already use feel sane on Windows.

The flagship archive extension is `.tzs`, which means standard tar plus Zstandard. It is the preferred default for new archives, not a proprietary lock-in format.

The program should remain a broad Windows archive utility with support for ZIP, 7z, tarballs, and inherited archive formats where practical.

The project succeeds if it makes tarballs worth downloading on Windows.

The project fails if it cannot extract tarballs effectively or if it introduces regressions in inherited 7-Zip behavior.

Build toward that.
