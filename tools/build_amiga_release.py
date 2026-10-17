#!/usr/bin/env python3
"""Build the four Raptor Amiga binaries and record their provenance.

The wrapper does not modify the game code or the existing build scripts. It:

1. verifies a clean source tree and records the exact commit before building;
2. runs the four existing build scripts sequentially;
3. checks each result and copies it to a protected directory outside the repo;
4. verifies that the source tree is unchanged after the builds;
5. runs the instruction checks and only then writes the build record consumed by
   tools/package_amiga_release.py.

It never tags, commits, pushes or publishes anything.

Example:
    python3 tools/build_amiga_release.py \
        --version 0.9.9-rc.6 \
        --bin-dir ~/raptor-release-0.9.9-rc.6/bin \
        --out ~/raptor-release-0.9.9-rc.6/build.json
"""

from __future__ import annotations

import argparse
import datetime
import json
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from package_amiga_release import (  # noqa: E402
    BINARIES,
    BINARY_NAMES,
    ReleaseError,
    fail,
    git,
    git_root,
    instruction_counts,
    is_amiga_executable,
    objdump_tool,
    resolve_commit,
    run,
    sha256_file,
    validate_instruction_checks,
    write_text,
    SCHEMA,
)

BUILD_STEPS = [
    ("build_amiga.sh", "raptor"),
    ("build_amiga_nofpu.sh", "raptor_nofpu"),
    ("build_amiga_030.sh", "raptor_030"),
    ("build_amiga_030_fpu.sh", "raptor_030_fpu"),
]


def version_line(command: str) -> str:
    proc = run([command, "--version"], check=False, timeout=60)
    if proc is None:
        return "unavailable"
    text = (proc.stdout or "") + (proc.stderr or "")
    for line in text.splitlines():
        if line.strip():
            return line.strip()[:300]
    return "unknown"


def is_inside(path: Path, parent: Path) -> bool:
    try:
        path.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        prog="build_amiga_release.py",
        description="Build the four Amiga binaries and write a provenance record.",
    )
    parser.add_argument("--version", required=True)
    parser.add_argument("--bin-dir", required=True, help="protected output directory")
    parser.add_argument("--out", required=True, help="build record JSON path")
    parser.add_argument("--repo", help="repository root (default: current directory)")
    args = parser.parse_args(argv)

    try:
        repo = git_root(Path(args.repo) if args.repo else Path.cwd())
        bin_dir = Path(args.bin_dir).resolve()
        if is_inside(bin_dir, repo):
            fail("--bin-dir must be outside the repository so the build tree stays clean")
        if bin_dir.exists() and any(bin_dir.iterdir()):
            fail("--bin-dir must be empty: %s" % bin_dir)

        status_before = git(repo, "status", "--porcelain")
        if (status_before or "").strip():
            fail(
                "the source tree must be clean before building:\n%s" % status_before.strip()
            )
        bin_dir.mkdir(parents=True, exist_ok=True)
        source_ref = resolve_commit(repo, "HEAD")
        tags = [line.strip() for line in git(repo, "tag", "--points-at", "HEAD").splitlines()]
        tag_name = args.version if args.version in tags else (tags[0] if tags else None)
        tag = None
        if tag_name:
            obj = git(repo, "rev-parse", "--verify", "refs/tags/%s" % tag_name).strip()
            tag = {
                "name": tag_name,
                "object": obj,
                "commit": resolve_commit(repo, tag_name),
                "type": git(repo, "cat-file", "-t", obj).strip(),
            }

        scripts = []
        for script, _binary in BUILD_STEPS:
            path = repo / script
            if not path.is_file():
                fail("missing build script: %s" % path)
            scripts.append({"path": script, "sha256": sha256_file(path)})

        objdump = objdump_tool()
        if objdump is None:
            fail("m68k-amigaos-objdump not found; cannot verify the build results")

        protected = {}
        for script, binary in BUILD_STEPS:
            print("=== %s ===" % script, flush=True)
            proc = subprocess.run(
                ["bash", script],
                cwd=str(repo),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            if proc.returncode != 0:
                print(proc.stdout, file=sys.stderr)
                fail("%s failed with exit code %d" % (script, proc.returncode))
            source = repo / binary
            if not source.is_file():
                fail("%s did not produce %s" % (script, binary))
            if not is_amiga_executable(source):
                fail("%s is not an AmigaOS loadseg()ble executable" % source)
            data = source.read_bytes()
            if args.version.encode("ascii") not in data:
                fail(
                    "%s does not contain the version string %r; bump the version before "
                    "building" % (source, args.version)
                )
            target = bin_dir / binary
            shutil.copyfile(source, target)
            target.chmod(0o755)
            if sha256_file(target) != sha256_file(source):
                fail("copy verification failed for %s" % binary)
            protected[binary] = target
        print("all four build scripts completed", flush=True)

        status_after = git(repo, "status", "--porcelain")
        if (status_after or "").strip():
            fail(
                "the source tree is not clean after building:\n%s" % status_after.strip()
            )
        after_ref = resolve_commit(repo, "HEAD")
        if after_ref != source_ref:
            fail("HEAD changed during the build: %s -> %s" % (source_ref, after_ref))
        if set(protected) != set(BINARY_NAMES):
            fail("not all four binaries were produced")

        checks = {}
        binaries = []
        for name, target in BINARIES:
            data = protected[name].read_bytes()
            counts = instruction_counts(protected[name], objdump)
            validate_instruction_checks(name, counts)
            checks[name] = counts
            binaries.append(
                {
                    "name": name,
                    "size": len(data),
                    "sha256": sha256_file(protected[name]),
                    "checks": counts,
                }
            )

        toolchain = {
            "built_with": "; ".join(
                [
                    "m68k-amigaos-gcc: %s" % version_line("m68k-amigaos-gcc"),
                    "m68k-amigaos-g++: %s" % version_line("m68k-amigaos-g++"),
                    "m68k-amigaos-objdump: %s" % version_line(objdump),
                ]
            ),
            "verified_with": {"m68k-amigaos-objdump": version_line(objdump)},
        }
        record = {
            "schema": SCHEMA,
            "mode": "built",
            "version": args.version,
            "source_ref": source_ref,
            "binaries_ref": source_ref,
            "tag": tag,
            "worktree": {"clean_before": True, "clean_after": True},
            "binaries": binaries,
            "checks": checks,
            "scripts": scripts,
            "toolchain": toolchain,
            "created": datetime.datetime.now(datetime.timezone.utc).strftime(
                "%Y-%m-%dT%H:%M:%SZ"
            ),
        }
        out = Path(args.out)
        out.parent.mkdir(parents=True, exist_ok=True)
        write_text(out, json.dumps(record, indent=2) + "\n")
        print("build record: %s" % out)
        print("source commit: %s (tag: %s)" % (source_ref, tag_name or "none"))
        for name in BINARY_NAMES:
            print("  %s  %s" % (sha256_file(protected[name]), name))
        print("source tree unchanged; no tag, commit, push or publication performed")
        return 0
    except ReleaseError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
