# Releasing Raptor for Amiga

This document is the canonical procedure for preparing a Raptor Amiga release.
It exists so that a future package can be produced repeatably without
reconstructing the process from chat history.

The tools never tag, commit, push or publish anything. Packaging a candidate
and publishing it are separate steps: publication happens only when it is
explicitly requested.

## Files involved

| Path | Purpose |
| --- | --- |
| `tools/build_amiga_release.py` | runs the four existing build scripts sequentially and writes a build record |
| `tools/package_amiga_release.py` | `record-reuse` and `package`: provenance, staging, LHA, descriptions, manifest, checksums, safe install |
| `packaging/amiga/icons/` | byte-exact icon templates imported once from the verified 0.9.9-rc.5 archive |
| `packaging/amiga/templates/` | Aminet readme, GitHub release notes and `release-info` templates |
| `tests/test_release_replace.py` | rollback test for the install step |
| `releases/` | local, git-ignored release output directory (not versioned) |

## Prerequisites

- `git`, Python 3.8 or newer, `sha256sum` and `cmp`.
- The m68k-amigaos toolchain in `/opt/amiga/bin` (`m68k-amigaos-gcc`,
  `m68k-amigaos-g++`, `m68k-amigaos-objdump`), as used by the build scripts.
- An **LHA writer**: Lhasa cannot write archives. In this environment
  `/usr/bin/lha` is Lhasa 0.5.0 and is usable only as a reader.
- An **LHA reader**: `lhasa`, or a binary named `lha` that is Lhasa. The
  reader is required for the independent integrity test and the control
  extraction.

### LHA writer: pinned source and verified syntax

The verified writer is LHa for UNIX, pinned to commit
`16619b066b189ef289bb8b07b37d1c38d550da99` of
<https://github.com/jca02266/lha>. The build in this environment reports:

```
LHa for UNIX version 1.14i-ac20260723 (x86_64-pc-linux-gnu)
```

Build it outside the repository, without installing it system-wide:

```
git clone https://github.com/jca02266/lha /tmp/raptor-lha
cd /tmp/raptor-lha
git checkout 16619b066b189ef289bb8b07b37d1c38d550da99
autoreconf -is
./configure --prefix=/tmp/raptor-lha-install
make -j2
# writer binary: /tmp/raptor-lha/src/lha
```

Verified command syntax (`a` add, `q` quiet, `-o5` = `-lh5-` method; note that
`-lh5` is not the option):

```
lha aq -o5 raptor.lha FILE...
```

The verified archive format is an LHA level-2 header with the `-lh5-` method:
the method field at offset 2 is `-lh5-` and the header-level byte at offset 20
is `2`. The packaging script parses the produced archive and refuses anything
else. Pass the writer explicitly:

```
--lha-writer /tmp/raptor-lha/src/lha
```

The reader is auto-detected (`lhasa`, then `lha`) or passed with
`--lha-reader`. Do not assume that a command named `lhasa` exists; on this
system the reader is `/usr/bin/lha`.

### Icon import (already done, do not repeat per release)

The icons in `packaging/amiga/icons/` are byte-for-byte copies from the
verified 0.9.9-rc.5 `raptor.lha`. They were parsed once with **amigainfo
0.6.1** (PyPI wheel `amigainfo-0.6.1-py3-none-any.whl`, run from `PYTHONPATH`
without installation, rendering dependency Pillow 13.0.0). The result is
`packaging/amiga/icons/verification.json`.

Packaging does not re-implement an icon parser. It verifies the pinned
SHA-256 values, the icon types, the ToolTypes and the presence of exactly one
inactive `(CD32 options: ON | OFF)` and one `(CD32=OFF)` entry in each program
icon. A custom `--icons-dir` must contain an equivalent `verification.json`.
Do not open and re-save the icons in an icon editor.

## Step 1 - prepare sources and version

Work on the commit that will become the release. Update, in one commit or a
small series:

- `CHANGELOG.md` - add a dated section `## [<version>] - YYYY-MM-DD` with the
  release content; the date is used in the release manifest;
- `README.md` - `Current version: **<version>**`;
- `README_AMIGA.md` - `Version: <version>`;
- `README_AMIGA_PL.md` - `Wersja: <version>`;
- `SOURCE.txt` - `For release <version>:` and the tag link;
- `src/rap.cpp` - the startup banner version.

The packaging script verifies all of these strings at the source commit and
refuses to package on a mismatch. The repository-root `raptor.readme` is not
used; the Aminet description is generated from the template and the release
information.

Choose a work directory outside the repository, for example
`~/raptor-release-<version>/`, with `bin/`, `build.json` and
`release-info.json`. Never stage release files inside the repository.

## Step 2 - configuration test

Run the host test for the `amiga.cfg` handling:

```
bash tests/run_amiga_cfg_test.sh
```

It must pass before building. It does not require Amiga hardware.

## Step 3 - four sequential builds with provenance

Run the wrapper from the release commit. `--bin-dir` must be outside the
repository and must be empty:

```
python3 tools/build_amiga_release.py \
    --version <version> \
    --bin-dir ~/raptor-release-<version>/bin \
    --out ~/raptor-release-<version>/build.json
```

The wrapper:

1. requires a clean working tree and records the exact `HEAD`;
2. runs, sequentially and stopping on the first failure:
   `build_amiga.sh`, `build_amiga_nofpu.sh`, `build_amiga_030.sh`,
   `build_amiga_030_fpu.sh`;
3. checks each result (AmigaOS HUNK magic, version string in the binary);
4. copies each result immediately into `--bin-dir`, so a later build cannot
   change an earlier result;
5. verifies that `HEAD` and the working tree are unchanged after the builds;
6. runs the instruction checks (`no FPU` for `raptor_nofpu` and `raptor_030`,
   `no byterev/bitrev` for the 68030 builds) and writes `build.json`.

The record is written only after all checks pass. Do not edit it by hand.
A fresh build record has `binaries_ref == source_ref` and proves a clean source
tree before and after the build. A version string or a file modification time
alone does not prove that a binary came from the source commit.

## Step 4 - binary provenance and reuse after documentation commits

A package may use binaries built from an earlier commit only when the
difference between that commit and the packaged source commit is on a
**conservative allow-list of paths that cannot influence the build**:

```
README.md
README_AMIGA.md
README_AMIGA_PL.md
CHANGELOG.md
SOURCE.txt
raptor.readme
LICENSE
docs/**
packaging/amiga/**
tools/build_amiga_release.py
tools/package_amiga_release.py
.clinerules/amiga-release.md
```

Anything else blocks automatic reuse: changes under `src/`, `include/`,
`cmake/`, the build scripts, `Makefile.amiga`, `CMakeLists.txt`,
`CMakePresets.json`, `tests/`, other files under `tools/`, additions,
deletions, renames and **any change to `.gitattributes`**. An unknown change
is treated as build-relevant; rebuild the binaries instead of guessing.

The reuse record is created by reading the previous release directory:

```
python3 tools/package_amiga_release.py record-reuse \
    --version <version> \
    --bin-dir ~/raptor-release-<version>/bin \
    --previous-release releases/<previous-version> \
    --out ~/raptor-release-<version>/build.json
```

It verifies the previous `SHA256SUMS.txt`, compares all four binaries with the
previous `MANIFEST.txt`, and records `mode: reused`. It preserves the previous
release's provenance (manifest hash, archive hash, source commit, build
commit) and records the current toolchain only as "verified with". It never
claims that the current toolchain built the old binaries.

`record-reuse` applies only to binaries that were already verified and
packaged. For a new release, build the binaries from the release commit.

## Step 5 - release information and packaging

Create `release-info.json` outside the repository. Start from
`packaging/amiga/templates/release-info.example.json`:

```json
{
  "uploader": "your-aminet-uploader@example.com",
  "previous_version": "0.9.9-rc.5",
  "summary": "One paragraph for Aminet and GitHub.",
  "whats_new_md": "Markdown. Optional: defaults to the CHANGELOG section.",
  "upgrade_notes_md": "Manual steps required when upgrading; leave empty if none.",
  "testing_status_md": "Exactly what was tested, where, and by whom."
}
```

Only `uploader` is mandatory. `previous_version` is derived from the highest
older tag when omitted. `whats_new_md` defaults to the CHANGELOG section of
the packaged version, so the descriptions are always based on the release
notes. `testing_status_md` defaults to "No new hardware testing is claimed for
this release."; never write a claim that was not actually verified.

Run:

```
python3 tools/package_amiga_release.py package \
    --version <version> \
    --source-ref "$(git rev-parse HEAD)" \
    --bin-dir ~/raptor-release-<version>/bin \
    --build-record ~/raptor-release-<version>/build.json \
    --release-info ~/raptor-release-<version>/release-info.json \
    --lha-writer /tmp/raptor-lha/src/lha \
    --out-dir releases/<version>
```

What the script does:

- resolves `--source-ref` to an exact commit and reads every document from
  that commit with `git`, not from the working tree;
- verifies the build record, the binary hashes/sizes, the HUNK magic, the
  instruction checks and the reuse allow-list;
- verifies the icon templates and their pinned SHA-256/import record;
- stages exactly the verified layout (`Raptor.info` plus `Raptor/`, four
  binaries, four program icons, `ReadMe.info`, and the six documents with
  their fixed archive names);
- creates `raptor.lha` with the pinned writer and verifies the method/header
  level;
- writes `raptor.readme` (Aminet), `RELEASE_NOTES_<version>.md` (GitHub),
  `MANIFEST.txt` and `SHA256SUMS.txt`;
- installs the five files only after all checks pass.

The working tree does not have to be clean at packaging time, because the
documents come from the commit and the binaries are proven by the build
record. Tooling files that are not committed are recorded as `dirty` with
their actual SHA-256, so a test run of a new script is possible without a
commit. This does not weaken the binary provenance: the build record is what
proves the source state at build time.

## Step 6 - verify the package

The script refuses success unless all of the following passed, but repeat the
independent checks after any manual intervention:

```
cd releases/<version>
sha256sum -c SHA256SUMS.txt
/tmp/raptor-lha/src/lha t raptor.lha    # writer integrity (CRC)
lhasa t raptor.lha                      # independent reader integrity
lhasa l raptor.lha                      # exactly 16 entries, verified order
```

The script also performs a full control extraction with the independent
reader into a temporary directory and compares every extracted file
byte-for-byte with the staged file (16/16, no extra files, no symlinks).
It rejects game data (`*.GLB`), music (`MP3/`, `WAVE/`), savegames, a personal
`amiga.cfg` (only `amiga.cfg.example` is allowed), logs, object files, `.git`,
tools and temporary files. All sums are relative to the release directory, so
`sha256sum -c SHA256SUMS.txt` works after downloading the five files into one
directory.

Inspect `MANIFEST.txt`: it records the source commit, the tag object (or
`none` for a candidate), the binary build commit, every binary SHA-256, the
icon import verification, the document sources and hashes, the tool/template
provenance, the archive listing and the performed checks.

## Step 7 - candidate, tag and publication

Packaging does not require a tag. A package made for an exact commit without
a tag is a **candidate**: `MANIFEST.txt` records `Tag: none at packaging time`
and the commit on which the tag must be created. If a tag with the version
name (or the name passed with `--tag`) already exists, its commit must equal
`--source-ref`; otherwise packaging fails. The script never creates, moves or
deletes a tag.

Suggested order:

1. Commit the version bump and documentation.
2. Package and verify the candidate.
3. Only when publication is explicitly requested: create an annotated tag on
   the exact packaged commit, push the branch and the tag (never with
   `--force`), publish the GitHub release and upload `raptor.lha` /
   `raptor.readme` to Aminet.

Tag creation is a separate, explicitly requested action:

```
git tag -a <version> -m "Raptor Amiga Port <version>" <source-commit>
git push origin main
git push origin <version>
```

Do not move an existing release tag. If the tag already exists, verify that
it points to the packaged commit and leave it untouched.

Publication to GitHub Releases and Aminet is **not** part of this procedure's
automatic path. It must be requested separately, after the package and the
manifest have been verified.

## Step 8 - what the tools never do

- They never run `git tag`, `git commit`, `git push` or any other state-changing
  git command.
- They never publish a GitHub release and never upload to Aminet.
- They never overwrite an existing release directory without an explicit
  `--force`. A directory containing anything other than exactly the five
  release files is always refused, and nothing in it is deleted.
- They never modify or reconstruct the icon templates.
- They never include game data, music, savegames, a personal `amiga.cfg`,
  logs, object files or temporary files.

## Safe replacement

The final install staging and the backup are created next to `--out-dir`, so
every `os.replace` happens on the same filesystem as the destination. The new
five-file set is fully built and verified before anything is replaced. The old
set is copied to a backup; on a failed replacement the old files are restored
and verified. If restoration itself fails, the backup is kept and its path is
reported. The rollback path is covered by
`python3 tests/test_release_replace.py`.

## Known limitations

- LHA archives are not byte-reproducible across different writer versions or
  packaging times; the archive content, the method (`-lh5-`), the header level
  (2) and every extracted file are verified instead.
- File timestamps inside the archive are the staging times. They are not part
  of the checksum contract.
- Unix mode bits are stored by the writer. The verified layout uses modes
  that keep files writable on the Amiga side; the Amiga protection bits are
  not the subject of the manifest.
- The icon templates use a neutral `game/` directory that maps to `Raptor/`
  inside the archive. This keeps `Raptor.info` and `Raptor/raptor.info`
  distinct and avoids the broad `raptor` pattern in `.gitignore` on
  case-insensitive filesystems.
- No hardware verification is implied by a successful packaging run.

## Quick command for the next release

After the version bump is committed, the four builds succeed and
`build.json` exists:

```
python3 tools/package_amiga_release.py package \
    --version <version> \
    --source-ref "$(git rev-parse HEAD)" \
    --bin-dir ~/raptor-release-<version>/bin \
    --build-record ~/raptor-release-<version>/build.json \
    --release-info ~/raptor-release-<version>/release-info.json \
    --lha-writer /tmp/raptor-lha/src/lha \
    --out-dir releases/<version>
```
