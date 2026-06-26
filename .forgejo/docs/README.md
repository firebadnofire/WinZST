# RTSP Viewer Forgejo Release Workflow

Use this checklist when maintaining this repository's `.forgejo/` release
workflow. The detailed workflow behavior is documented in
[`release-packaging-workflow.md`](release-packaging-workflow.md).

## Plan

1. Keep `.forgejo/workflows/release-packaging.yml` aligned with RTSP Viewer build
   helper outputs and artifact names.
2. Confirm the Forgejo runner fleet has the labels used by the matrix:
   `macos-latest` and `ubuntu-22.04`.
3. Confirm each runner has the required native or cross toolchain before pushing
   a tag.
4. Confirm the `GH_KEY` secret can push to and create releases in
   `github.com/firebadnofire/rtsp-webview`.
5. Validate the workflow syntax and at least one local build path before pushing
   a release tag.

## Implementation

### Release Assets

The workflow publishes these deterministic asset names:

```text
rtsp-viewer-windows-amd64.zip
rtsp-viewer-macos-app.zip (universal Intel/Apple Silicon app)
rtsp-viewer-linux-x86_64.tar.gz
```

Search workflow files for copied Android or source-project assumptions after
edits:

```bash
rg -n 'APK|apk|Android|android|Launch Pad|launchpad|Gradle|gradlew|KEYSTORE|setup-java|setup-android|actions/checkout|uses:' .forgejo/workflows
```

### Runner Requirements

macOS:

- `bash`, `git`, `base64`, `curl`, `jq`
- Node.js 24 LTS as `node`, plus `npm`, `cargo`, `rustup`
- Xcode Command Line Tools
- `lipo`
- `codesign`
- `ditto`
- host-mode runner service PATH must expose Rust and Homebrew tools, or the
  workflow must be able to find them in `~/.cargo/bin`, `/opt/homebrew/bin`, or
  `/usr/local/bin`

Linux:

- `bash`, `git`, `base64`, `curl`, `jq`
- `apt-get` access to install native Tauri/WebKit build dependencies
- `mingw-w64` and `zip` from Ubuntu packages for the Windows cross-build
- `x86_64-w64-mingw32-gcc` from `mingw-w64`
- Rust target `x86_64-pc-windows-gnu`
- outbound HTTPS access to NodeSource and rustup if Node 24 or Rust is missing

### Secrets

The workflow uses the Forgejo-provided workflow token for checkout and Forgejo
release publishing:

```text
${{ forgejo.token }}
```

It also requires this repository or organization secret:

```text
GH_KEY
```

`GH_KEY` must be able to push refs to `github.com/firebadnofire/rtsp-webview`,
create the repository if it is missing, and create/update GitHub releases and
release assets. No Android signing secrets, keystore, or APK-specific secret is
required.

## Validation

Run these checks in the target repository before pushing a release tag:

```bash
ruby -e 'require "yaml"; YAML.load_file(".forgejo/workflows/release-packaging.yml"); puts "yaml ok"'
rg -n 'APK|apk|Android|android|Launch Pad|launchpad|Gradle|gradlew|KEYSTORE|setup-java|setup-android|actions/checkout|uses:' .forgejo/workflows
npm --prefix ui ci
npm --prefix ui run build
cargo test --locked --workspace
rustup target add x86_64-pc-windows-gnu
cargo check --locked --release --target x86_64-pc-windows-gnu -p rtsp_viewer_tauri
```

The grep command should return no matches.

After validation, push a version tag:

```bash
git tag vX.Y.Z
git push origin vX.Y.Z
```

If Forgejo release publishing fails with a 404, check `FORGEJO_API_URL`,
`FORGEJO_REPOSITORY`, and whether the workflow token can create or update
releases for the repository. If GitHub mirroring fails with a 404 or 403, check
`GH_KEY` permissions and the `GITHUB_MIRROR_REPOSITORY` value in the workflow.
