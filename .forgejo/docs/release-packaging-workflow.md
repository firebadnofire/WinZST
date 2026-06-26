# Release Packaging Workflow

This document explains `.forgejo/workflows/release-packaging.yml` for future
maintenance. The workflow builds Windows, macOS, and Linux release assets for
RTSP Viewer when a version tag is pushed, publishes each asset to the matching
Forgejo release, mirrors Git refs to GitHub, and uploads each asset to the
matching GitHub release.

## Trigger

The workflow runs on pushed tags matching either lowercase or uppercase version
prefixes:

```yaml
'on':
  push:
    tags:
      - 'v*'
      - 'V*'
```

Examples that trigger the workflow:

- `v1.0.4`
- `V1.0.4`
- `v2.0.0-beta1`

## Runtime Environment

The workflow is a two-entry matrix:

| Matrix entry | Runner label | Build path | Asset(s) |
| --- | --- | --- | --- |
| macOS | `macos-latest` | `./build-helpers/mac/build-app.sh` | `rtsp-viewer-macos-app.zip` universal app |
| Linux | `ubuntu-22.04` | Native Linux build plus Windows GNU cross-build | `rtsp-viewer-linux-x86_64.tar.gz`, `rtsp-viewer-windows-amd64.zip` |

The runner labels are infrastructure configuration. They must exist on the
Forgejo runner fleet and must map to a native macOS runner and an Ubuntu Linux
runner. This repository cannot verify runner labels locally.

## Checkout

The workflow does not use `actions/checkout`. It first prepares the minimal
checkout runtime, then performs checkout with shell commands against:

```text
${FORGEJO_SERVER_URL}/${FORGEJO_REPOSITORY}.git
```

The Linux matrix entry installs `ca-certificates`, `coreutils`, `curl`, `git`,
and `jq` with `apt-get` when any of those tools are missing. The macOS matrix
entry expects Xcode Command Line Tools or equivalent runner provisioning to
provide `git`.

Host-mode macOS runners often start non-login shells that do not inherit the
interactive terminal PATH. The workflow explicitly adds `~/.cargo/bin`,
`/opt/homebrew/bin`, and `/usr/local/bin` before validating tools and before
running the macOS build helper.

The checkout step requires HTTPS and fails for non-HTTPS server URLs. It uses
the Forgejo workflow token as an HTTP extra header when a token is present. This
avoids relying on `actions/*` mirrors or automatic action rewrites in a
self-hosted Forgejo environment.

## Build Outputs

macOS:

1. Runs the repository macOS helper.
2. Builds `dist/macos/RTSP Viewer.app` with a universal `x86_64` and `arm64`
   executable.
3. Creates `dist/releases/rtsp-viewer-macos-app.zip` with `ditto`, preserving app
   bundle metadata.

Linux:

1. Installs native Ubuntu packages needed to compile the Tauri WebKitGTK app.
2. Installs `mingw-w64` and `zip` for Windows GNU cross-compilation and ZIP
   packaging.
3. Installs Node 24 from NodeSource if the runner does not already have Node 24.
4. Installs stable Rust through rustup if `cargo` or `rustup` is missing.
5. Adds Rust target `x86_64-pc-windows-gnu`.
6. Verifies `x86_64-w64-mingw32-gcc` exists and exports it as the target linker.
7. Runs `npm --prefix ui ci`, `npm --prefix ui run build`, and
   `cargo build --locked --release -p rtsp_viewer_tauri`.
8. Runs
   `cargo build --locked --release --target x86_64-pc-windows-gnu -p rtsp_viewer_tauri`.
9. Packs the native Linux binary, license, and Linux runtime notes into
   `dist/releases/rtsp-viewer-linux-x86_64.tar.gz`.
10. Copies the Windows GNU build output to `dist/windows/amd64/rtsp-viewer.exe`.
11. Copies the GNU-target WebView2 loader DLL to the same directory.
12. Packages both files as `dist/releases/rtsp-viewer-windows-amd64.zip`.

Each job validates that all configured release artifacts exist and are non-empty before
publishing.

## Forgejo Release Publishing

Each matrix job publishes its own artifact to the Forgejo release for the tag.
The publish step:

1. Derives the API URL, repository, tag, and token from Forgejo environment
   variables with GitHub-compatible fallbacks.
2. Requires an HTTPS API URL.
3. Looks up the release by tag.
4. Creates the release if it is missing.
5. Patches the release if it already exists.
6. Deletes an existing asset with the same deterministic filename.
7. Uploads the new asset.

The release publishing steps are safe to rerun for the same tag because release
metadata is patched and same-name assets are replaced.

## GitHub Mirroring

The workflow mirrors to:

```text
github.com/firebadnofire/rtsp-webview
```

The target is controlled by:

```yaml
env:
  GITHUB_MIRROR_REPOSITORY: firebadnofire/rtsp-webview
```

Each matrix job mirrors refs after its Forgejo asset upload succeeds. The mirror
step:

1. Requires `GH_KEY`.
2. Checks whether the GitHub repository exists.
3. Creates the repository if it is missing and the token is authorized to create
   it.
4. Fetches all Forgejo branches and tags from the HTTPS Forgejo remote.
5. Pushes Forgejo branches to GitHub branches.
6. Pushes all Forgejo tags to GitHub tags.

The push is intentionally not forced. Divergent GitHub branches or tags fail the
job so history mismatches are visible and can be resolved manually.

## GitHub Release Publishing

Each matrix job uploads its own artifact to the GitHub release for the same tag.
The GitHub release step:

1. Requires `GH_KEY`.
2. Looks up the GitHub release by tag.
3. Creates it if missing.
4. Patches it if present.
5. Sets `draft: false`, `prerelease: false`, and `make_latest: "true"`.
6. Deletes an existing same-name asset.
7. Uploads the new asset through `uploads.github.com`.

The GitHub release steps are safe to rerun for the same tag because same-name
assets are replaced.

## External Dependencies

Validated from this workspace on April 30, 2026:

| Dependency | Exact reference | Validation |
| --- | --- | --- |
| Forgejo/Gitea API | `https://pubcode.archuser.org/api/v1/version` | Returned `14.0.4+gitea-1.22.0`. |
| Repository HTTPS remote | `https://pubcode.archuser.org/firebadnofire/rtsp-webview.git` | `git ls-remote` returned `HEAD` and tag refs. |
| GitHub API | `https://api.github.com/` | HTTPS HEAD returned `200`. |
| GitHub upload API | `https://uploads.github.com/` | HTTPS HEAD returned `302` to `https://github.com`, confirming the host resolves. |
| GitHub mirror repository API | `https://api.github.com/repos/firebadnofire/rtsp-webview` | Returned repository JSON for `firebadnofire/rtsp-webview`. |
| GitHub mirror Git remote | `https://github.com/firebadnofire/rtsp-webview.git` | `git ls-remote` returned `HEAD`, `main`, and tag refs. |
| Ubuntu mingw-w64 package | `https://packages.ubuntu.com/search?keywords=mingw-w64&searchon=names&suite=jammy&section=all` | HTTPS package search returned an exact `mingw-w64` hit for jammy. |
| Ubuntu zip package | `https://packages.ubuntu.com/jammy/amd64/zip/download` | HTTPS package download page returned `200`. |
| NodeSource GPG key | `https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key` | HTTPS HEAD returned `200`. |
| NodeSource Node 24 repository | `https://deb.nodesource.com/node_24.x/dists/nodistro/Release` | HTTPS HEAD returned `200`. |
| Rust installer | `https://sh.rustup.rs` | HTTPS HEAD returned `200`. |
| npm registry | `https://registry.npmjs.org/` | HTTPS HEAD returned `200`. |
| crates.io API | `https://crates.io/api/v1/crates/serde` | HTTPS API request returned crate JSON. |
| Debian package repository | `https://deb.debian.org/debian/` | HTTPS HEAD returned `200`. |
| Debian security repository | `https://security.debian.org/debian-security/` | HTTPS HEAD returned `200`. |

Not locally verifiable from this workspace:

- Forgejo runner labels: `macos-latest`, `ubuntu-22.04`.
- Tools installed on those target runners.
- The permissions granted to `${{ forgejo.token }}` during workflow execution.
- The permissions granted to `GH_KEY` during workflow execution.

If any of these are unavailable on the target Forgejo instance, update the matrix
runner labels or runner provisioning before pushing a release tag.

## Security Notes

- The workflow requires HTTPS for repository checkout and release publishing.
- `curl` calls restrict protocols to HTTPS and disable deprecated TLS versions
  below TLS 1.2 while allowing TLS 1.3 when the server negotiates it.
- No release credentials are hardcoded.
- `GH_KEY` is read only from workflow secrets and is used for GitHub repository
  mirroring and GitHub release asset publishing.
- No Android keystore, signing password, or APK-specific secret is used.
- The workflow does not use third-party `uses:` actions, which avoids ambiguous
  Forgejo action mirror behavior.

## Validation

Before pushing a release tag, run:

```bash
ruby -e 'require "yaml"; YAML.load_file(".forgejo/workflows/release-packaging.yml"); puts "yaml ok"'
rg -n 'APK|apk|Android|android|Launch Pad|launchpad|Gradle|gradlew|KEYSTORE|setup-java|setup-android|actions/checkout|uses:' .forgejo/workflows
npm --prefix ui ci
npm --prefix ui run build
cargo test --locked --workspace
rustup target add x86_64-pc-windows-gnu
cargo check --locked --release --target x86_64-pc-windows-gnu -p rtsp_viewer_tauri
```

The grep command should return no matches. The full release matrix still requires
the Forgejo runner fleet because macOS remains a native build and the Linux job
performs both native Linux packaging and Windows GNU cross-compilation.
