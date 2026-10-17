#!/usr/bin/env python3
"""Prepare a reproducible Raptor Amiga release package.

The tool never tags, commits, pushes or publishes anything. It only reads the
repository (documents come from an explicit source commit), verifies binaries
against a recorded provenance file, stages the release outside the repository,
creates raptor.lha and writes the release description, notes, manifest and
checksums.

Subcommands
-----------
record-reuse
    Record that the binaries in --bin-dir are the already verified binaries
    from a previous release directory. The previous provenance is preserved;
    the currently installed toolchain is recorded only as "verified with".

package
    Build the release set for --version from --source-ref, verify it and
    install it into --out-dir.

See docs/releasing-amiga.md for the full process.
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

SCHEMA = 1

PROJECT_URL = "https://github.com/RaybeezPL/raptor-amiga-port"

# name, human readable target
BINARIES = [
    ("raptor", "68060 with FPU (recommended)"),
    ("raptor_nofpu", "68060 / 68EC060 / 68LC060 soft-float"),
    ("raptor_030_fpu", "68030 with external 68881/68882 FPU"),
    ("raptor_030", "68030 soft-float"),
]
BINARY_NAMES = [name for name, _target in BINARIES]

# source path in the repository, path inside the archive
DOCUMENTS = [
    ("README_AMIGA.md", "Raptor/ReadMe"),
    ("README_AMIGA_PL.md", "Raptor/ReadMePL.txt"),
    ("CHANGELOG.md", "Raptor/CHANGELOG.md"),
    ("docs/amiga.cfg.example", "Raptor/amiga.cfg.example"),
    ("SOURCE.txt", "Raptor/SOURCE.txt"),
    ("LICENSE", "Raptor/LICENSE"),
]

# template path below the icons directory, path inside the archive
ICONS = [
    ("Raptor.info", "Raptor.info"),
    ("game/ReadMe.info", "Raptor/ReadMe.info"),
    ("game/raptor.info", "Raptor/raptor.info"),
    ("game/raptor_nofpu.info", "Raptor/raptor_nofpu.info"),
    ("game/raptor_030.info", "Raptor/raptor_030.info"),
    ("game/raptor_030_fpu.info", "Raptor/raptor_030_fpu.info"),
]
PROGRAM_ICONS = {path for _src, path in ICONS if path.startswith("Raptor/raptor")}
PROGRAM_ICON_TEMPLATES = {
    template for template, target in ICONS if target in PROGRAM_ICONS
}
ICON_TYPES = {
    "Raptor.info": "drawer",
    "game/ReadMe.info": "project",
    "game/raptor.info": "tool",
    "game/raptor_nofpu.info": "tool",
    "game/raptor_030.info": "tool",
    "game/raptor_030_fpu.info": "tool",
}

CD32_OPTIONS_TT = "(CD32 options: ON | OFF)"
CD32_OFF_TT = "(CD32=OFF)"

# Conservative allow-list for reusing binaries built from an older commit.
# Only paths that cannot influence the Amiga build result are accepted.
# Anything else (including .gitattributes, tests/, other tools/* and new
# unknown files) blocks automatic reuse.
REUSE_ALLOWED_FILES = {
    "README.md",
    "README_AMIGA.md",
    "README_AMIGA_PL.md",
    "CHANGELOG.md",
    "SOURCE.txt",
    "raptor.readme",
    "LICENSE",
    "tools/build_amiga_release.py",
    "tools/package_amiga_release.py",
    ".clinerules/amiga-release.md",
}
REUSE_ALLOWED_PREFIXES = ("docs/", "packaging/amiga/")



class ReleaseError(Exception):
    """A controlled failure with a user readable message."""


def fail(message: str) -> None:
    raise ReleaseError(message)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(cmd, cwd=None, check=True, timeout=900, binary=False):
    """Run a command and return CompletedProcess with bytes or text output."""
    try:
        proc = subprocess.run(
            [str(part) for part in cmd],
            cwd=str(cwd) if cwd else None,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            text=not binary,
        )
    except FileNotFoundError:
        if check:
            fail("command not found: %s" % cmd[0])
        return None
    except subprocess.TimeoutExpired:
        if check:
            fail("command timed out: %s" % " ".join(str(c) for c in cmd))
        return None
    if check and proc.returncode != 0:
        detail = (proc.stderr or proc.stdout or "").strip()
        if isinstance(detail, bytes):
            detail = detail.decode("utf-8", "replace")
        fail(
            "command failed (%s): %s%s"
            % (
                proc.returncode,
                " ".join(str(c) for c in cmd),
                "\n" + detail if detail else "",
            )
        )
    return proc


def git(repo: Path, *args, check=True, binary=False):
    proc = run(["git", "-C", str(repo), *args], check=check, binary=binary)
    if proc is None:
        return None
    if binary:
        return proc.stdout
    return proc.stdout


def git_root(path: Path) -> Path:
    proc = run(["git", "-C", str(path), "rev-parse", "--show-toplevel"], check=False)
    if proc is None or proc.returncode != 0:
        fail("not inside a git repository: %s" % path)
    return Path(proc.stdout.strip()).resolve()


def resolve_commit(repo: Path, ref: str) -> str:
    proc = git(repo, "rev-parse", "--verify", "%s^{commit}" % ref, check=False)
    if not proc or not proc.strip():
        fail("cannot resolve git revision: %s" % ref)
    return proc.strip()


def tag_info(repo: Path, tag: str):
    obj = git(repo, "rev-parse", "--verify", "refs/tags/%s" % tag, check=False)
    if not obj or not obj.strip():
        return None
    obj = obj.strip()
    commit = resolve_commit(repo, tag)
    kind = git(repo, "cat-file", "-t", obj).strip()
    return {"name": tag, "object": obj, "commit": commit, "type": kind}


def version_key(value: str):
    match = re.match(r"^(\d+)\.(\d+)(?:\.(\d+))?(?:-rc\.(\d+))?$", value)
    if not match:
        return None
    major, minor, patch, rc = match.groups()
    return (
        int(major),
        int(minor),
        int(patch or 0),
        0 if rc is not None else 1,
        int(rc or 0),
    )


def derive_previous_version(repo: Path, version: str) -> str:
    current = version_key(version)
    if current is None:
        fail("cannot derive the previous version from an unusual version string: %s" % version)
    best = None
    best_key = None
    for tag in git(repo, "tag", "--list").splitlines():
        key = version_key(tag.strip())
        if key is None or key >= current:
            continue
        if best_key is None or key > best_key:
            best, best_key = tag.strip(), key
    if best is None:
        fail("cannot derive the previous version; add previous_version to --release-info")
    return best


def is_allowed_reuse_path(path: str) -> bool:
    if path in REUSE_ALLOWED_FILES:
        return True
    return any(path.startswith(prefix) for prefix in REUSE_ALLOWED_PREFIXES)


def changed_paths(repo: Path, old: str, new: str):
    output = git(repo, "diff", "--name-status", "-M", "--no-ext-diff", old, new, "--")
    changes = []
    for line in output.splitlines():
        if not line.strip():
            continue
        parts = line.split("\t")
        status = parts[0].strip()
        paths = [part.strip() for part in parts[1:] if part.strip()]
        if not paths:
            fail("cannot parse git diff entry: %s" % line)
        changes.append((status, paths))
    return changes


def check_reuse_allowed(repo: Path, binaries_ref: str, source_ref: str):
    if binaries_ref == source_ref:
        return []
    ancestor = run(
        ["git", "-C", str(repo), "merge-base", "--is-ancestor", binaries_ref, source_ref],
        check=False,
    )
    if ancestor is None or ancestor.returncode != 0:
        fail("binaries_ref %s is not an ancestor of %s" % (binaries_ref, source_ref))
    changes = changed_paths(repo, binaries_ref, source_ref)
    blocked = []
    for status, paths in changes:
        if status.startswith(("A", "M", "D", "R", "C", "T")):
            for path in paths:
                if not is_allowed_reuse_path(path):
                    blocked.append("%s %s" % (status, path))
        else:
            blocked.append("%s %s" % (status, " ".join(paths)))
    if blocked:
        fail(
            "binaries were built from %s and cannot be reused for %s; these changes are "
            "not on the non-build allow-list:\n  %s\nRebuild the binaries from the "
            "release commit." % (binaries_ref, source_ref, "\n  ".join(blocked))
        )
    return changes


def file_provenance(repo: Path, path: Path):
    path = path.resolve()
    data = path.read_bytes()
    entry = {
        "file": path.name,
        "sha256": sha256_bytes(data),
        "size": len(data),
        "commit": None,
        "dirty": True,
        "external": True,
    }
    try:
        rel = path.relative_to(repo.resolve()).as_posix()
    except ValueError:
        return entry
    entry["file"] = rel
    entry["external"] = False
    status = git(repo, "status", "--porcelain", "--", rel, check=False)
    exists = run(["git", "-C", str(repo), "cat-file", "-e", "HEAD:%s" % rel], check=False)
    if (not status or not status.strip()) and exists is not None and exists.returncode == 0:
        entry["commit"] = resolve_commit(repo, "HEAD")
        entry["dirty"] = False
    return entry


def read_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        fail("cannot read %s: %s" % (path, exc))
    except json.JSONDecodeError as exc:
        fail("invalid JSON in %s: %s" % (path, exc))


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8", newline="\n")


def normalize_text(text: str) -> str:
    lines = [line.rstrip() for line in text.replace("\r\n", "\n").split("\n")]
    result = []
    blank = 0
    for line in lines:
        if line == "":
            blank += 1
            if blank > 1:
                continue
        else:
            blank = 0
        result.append(line)
    return "\n".join(result).strip("\n") + "\n"



def git_show_bytes(repo: Path, ref: str, path: str) -> bytes:
    proc = run(
        ["git", "-C", str(repo), "cat-file", "blob", "%s:%s" % (ref, path)],
        check=False,
        binary=True,
    )
    if proc is None or proc.returncode != 0:
        detail = (proc.stderr or b"").decode("utf-8", "replace") if proc else ""
        fail("cannot read %s from %s: %s" % (path, ref, detail.strip()))
    return proc.stdout


def git_show_text(repo: Path, ref: str, path: str) -> str:
    return git_show_bytes(repo, ref, path).decode("utf-8")


def changelog_section(changelog: str, version: str):
    pattern = re.compile(
        r"^## \[%s\] - (\d{4}-\d{2}-\d{2})\s*$" % re.escape(version), re.MULTILINE
    )
    match = pattern.search(changelog)
    if not match:
        return None
    start = match.end()
    following = re.search(r"^## ", changelog[start:], re.MULTILINE)
    body = changelog[start : start + following.start()] if following else changelog[start:]
    return match.group(1), body.strip("\n")


def strip_inline_markdown(line: str) -> str:
    line = re.sub(r"!\[([^\]]*)\]\([^)]*\)", r"\1", line)
    line = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r"\1 (\2)", line)
    line = line.replace("**", "").replace("__", "")
    line = re.sub(r"`([^`]*)`", r"\1", line)
    line = re.sub(r"(?<![\w*])\*([^*\n]+)\*(?![\w*])", r"\1", line)
    return line


def markdown_to_text(markdown: str, width: int = 75) -> str:
    output = []
    in_code = False
    for raw in markdown.replace("\r\n", "\n").split("\n"):
        line = raw.rstrip()
        if line.strip().startswith("```"):
            in_code = not in_code
            continue
        if in_code:
            output.append(line)
            continue
        heading = re.match(r"^(#{1,6})\s+(.*?)\s*#*$", line)
        if heading:
            title = strip_inline_markdown(heading.group(2)).strip()
            if title:
                if output and output[-1] != "":
                    output.append("")
                output.append(title)
                output.append("")
            continue
        bullet = re.match(r"^(\s*)[-*+]\s+(.*)$", line)
        if bullet:
            indent = len(bullet.group(1))
            text = strip_inline_markdown(bullet.group(2)).strip()
            wrapped = textwrap.wrap(
                text,
                width=width,
                initial_indent=" " * indent + "- ",
                subsequent_indent=" " * (indent + 2),
            )
            output.extend(wrapped or [""])
            continue
        if not line.strip():
            output.append("")
            continue
        output.extend(textwrap.wrap(strip_inline_markdown(line), width=width) or [""])
    return "\n".join(output)


def wrap_paragraphs(text: str, width: int = 75) -> str:
    paragraphs = re.split(r"\n\s*\n", text.strip())
    rendered = []
    for paragraph in paragraphs:
        rendered.append("\n".join(textwrap.wrap(paragraph, width=width)))
    return "\n\n".join(rendered)


def render_template(template: str, values) -> str:
    names = set(re.findall(r"\{\{([A-Z0-9_]+)\}\}", template))
    unknown = names - set(values)
    if unknown:
        fail("template contains unknown placeholders: %s" % ", ".join(sorted(unknown)))
    rendered = template
    for key, value in values.items():
        rendered = rendered.replace("{{%s}}" % key, value)
    return normalize_text(rendered)


def load_release_info(path):
    if path is None:
        return {}
    info = read_json(path)
    if not isinstance(info, dict):
        fail("release-info must be a JSON object: %s" % path)
    known = {
        "uploader",
        "previous_version",
        "summary",
        "whats_new_md",
        "upgrade_notes_md",
        "testing_status_md",
        "_comment",
    }
    unknown = set(info) - known
    if unknown:
        fail("unknown release-info keys: %s" % ", ".join(sorted(unknown)))
    return info


def repository_url(repo: Path) -> str:
    url = git(repo, "remote", "get-url", "origin", check=False)
    url = (url or "").strip()
    if url.startswith("git@") and ":" in url:
        host, path = url[4:].split(":", 1)
        url = "https://%s/%s" % (host, path)
    if url.endswith(".git"):
        url = url[:-4]
    if not url.startswith(("http://", "https://")):
        return PROJECT_URL
    return url


def source_url(repo: Path, tag_name, commit: str) -> str:
    return "%s/tree/%s" % (repository_url(repo), tag_name or commit)


def check_version_at_ref(repo: Path, ref: str, version: str):
    checks = []

    def require(path: str, needle: str, label: str) -> str:
        text = git_show_text(repo, ref, path)
        if needle not in text:
            fail("%s at %s does not contain %r (expected %s)" % (path, ref, needle, label))
        checks.append("%s: %s" % (path, label))
        return text

    require("src/rap.cpp", version, "startup banner version")
    require("README.md", "Current version: **%s**" % version, "current version line")
    require("README_AMIGA.md", "Version: %s" % version, "documented version")
    require("README_AMIGA_PL.md", "Wersja: %s" % version, "documented Polish version")
    require("SOURCE.txt", "For release %s:" % version, "release tag reference")
    changelog = require("CHANGELOG.md", "## [%s]" % version, "changelog section")
    section = changelog_section(changelog, version)
    if section is None:
        fail("CHANGELOG.md has no '## [%s] - YYYY-MM-DD' heading" % version)
    release_date, body = section
    if not body.strip():
        fail("CHANGELOG.md section for %s is empty" % version)
    checks.append("CHANGELOG.md: dated section %s" % release_date)
    return release_date, body, checks



def load_icon_verification(icons_dir: Path):
    record_path = icons_dir / "verification.json"
    if not record_path.is_file():
        fail(
            "missing %s; icons must be imported once from the verified release "
            "archive with amigainfo and pinned there" % record_path
        )
    record = read_json(record_path)
    if record.get("schema") != 1:
        fail("unsupported icon verification schema in %s" % record_path)
    icons = record.get("icons")
    if not isinstance(icons, list):
        fail("icon verification record has no icons list")
    by_file = {}
    for entry in icons:
        if not isinstance(entry, dict) or "file" not in entry:
            fail("malformed icon verification entry in %s" % record_path)
        by_file[entry["file"]] = entry
    expected = {template for template, _target in ICONS}
    if set(by_file) != expected:
        fail(
            "icon templates and verification record differ; missing: %s, extra: %s"
            % (
                ", ".join(sorted(expected - set(by_file))) or "none",
                ", ".join(sorted(set(by_file) - expected)) or "none",
            )
        )
    for template, _target in ICONS:
        path = icons_dir / template
        entry = by_file[template]
        if not path.is_file():
            fail("missing icon template: %s" % path)
        if sha256_file(path) != entry.get("sha256"):
            fail("icon template %s does not match its pinned SHA-256" % path)
        if path.stat().st_size != entry.get("size"):
            fail("icon template %s does not match its pinned size" % path)
        if entry.get("type") != ICON_TYPES[template]:
            fail(
                "icon template %s has type %r, expected %r"
                % (template, entry.get("type"), ICON_TYPES[template])
            )
    for template in sorted(PROGRAM_ICON_TEMPLATES):
        tooltypes = by_file[template].get("tooltypes") or []
        if tooltypes.count(CD32_OPTIONS_TT) != 1 or tooltypes.count(CD32_OFF_TT) != 1:
            fail(
                "icon %s must contain the inactive entries %r and %r exactly once"
                % (template, CD32_OPTIONS_TT, CD32_OFF_TT)
            )
        for active in ("CD32=ON", "CD32=OFF"):
            if active in tooltypes:
                fail("icon %s contains the active entry %r" % (template, active))
        if "JOYSTICK=ON" not in tooltypes:
            fail("icon %s no longer contains JOYSTICK=ON" % template)
    return record


def is_amiga_executable(path: Path) -> bool:
    with open(path, "rb") as handle:
        return handle.read(4) == b"\x00\x00\x03\xf3"



def objdump_tool():
    candidates = [shutil.which("m68k-amigaos-objdump"), "/opt/amiga/bin/m68k-amigaos-objdump"]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return candidate
    return None


def instruction_counts(path: Path, objdump: str):
    proc = run([objdump, "-d", str(path)], check=False)
    if proc is None or proc.returncode != 0:
        fail("cannot disassemble %s with %s" % (path, objdump))
    fpu = 0
    m040 = 0
    for line in proc.stdout.splitlines():
        parts = line.split("\t")
        if len(parts) < 3:
            continue
        operation = parts[2].strip().split(" ")[0]
        if re.match(r"^f[a-z]", operation):
            fpu += 1
        elif operation in ("byterev", "bitrev"):
            m040 += 1
    return {"fpu_instructions": fpu, "m040_only_instructions": m040}


def validate_instruction_checks(name: str, counts) -> None:
    if name in ("raptor_nofpu", "raptor_030") and counts["fpu_instructions"] != 0:
        fail("%s contains %d FPU instructions" % (name, counts["fpu_instructions"]))
    if name in ("raptor_030", "raptor_030_fpu") and counts["m040_only_instructions"] != 0:
        fail(
            "%s contains %d 68040/68060-only instructions"
            % (name, counts["m040_only_instructions"])
        )


def toolchain_line(cmd: str) -> str:
    """Return a short first line from --version, without configure paths."""
    proc = run([cmd, "--version"], check=False, timeout=60)
    if proc is None:
        return "unavailable"
    text = (proc.stdout or "") + (proc.stderr or "")
    for line in text.splitlines():
        line = line.strip()
        if line:
            return line[:300]
    return "unknown"


def tool_version(cmd: str) -> str:
    """Return a short tool banner without configure paths or error text."""
    for arguments in ([], ["--version"], ["-v"]):
        proc = run([cmd, *arguments], check=False, timeout=30)
        if proc is None:
            continue
        text = (proc.stdout or "") + (proc.stderr or "")
        for line in text.splitlines():
            line = line.strip()
            if not line:
                continue
            if arguments == []:
                return line[:300]
            if "version" in line.lower() and "error" not in line.lower():
                return line[:300]
    return "unknown"


def find_writer(candidate):
    candidates = []
    if candidate:
        candidates.append(candidate)
    for name in ("lha", "lha-for-unix"):
        path = shutil.which(name)
        if path and path not in candidates:
            candidates.append(path)
    attempts = []
    for cmd in candidates:
        for options in (["-o5"], []):
            probe_dir = Path(tempfile.mkdtemp(prefix="raptor-lha-probe-"))
            try:
                (probe_dir / "probe.txt").write_bytes(b"raptor release probe\n" * 4096)
                archive = probe_dir / "probe.lha"
                proc = run(
                    [cmd, "aq", *options, str(archive), "probe.txt"],
                    cwd=probe_dir,
                    check=False,
                    timeout=60,
                )
                if proc is None or proc.returncode != 0 or not archive.is_file():
                    attempts.append("%s %s" % (cmd, " ".join(options) or "(default method)"))
                    continue
                data = archive.read_bytes()
                if len(data) >= 22 and data[2:7] == b"-lh5-" and data[20] == 2:
                    return cmd, options
                attempts.append("%s produced an unexpected archive header" % cmd)
            finally:
                shutil.rmtree(probe_dir, ignore_errors=True)
    fail(
        "no LHA writer found. Lhasa (often installed as /usr/bin/lha) can only read "
        "archives. Build LHa for UNIX from https://github.com/jca02266/lha at the "
        "pinned commit documented in docs/releasing-amiga.md and pass it with "
        "--lha-writer. Attempts: %s" % ("; ".join(attempts) or "none")
    )


def find_reader(candidate, archive: Path):
    candidates = []
    if candidate:
        candidates.append(candidate)
    for name in ("lhasa", "lha"):
        path = shutil.which(name)
        if path and path not in candidates:
            candidates.append(path)
    for cmd in candidates:
        test = run([cmd, "t", str(archive)], check=False, timeout=60)
        listing = run([cmd, "l", str(archive)], check=False, timeout=60)
        if (
            test is not None
            and listing is not None
            and test.returncode == 0
            and listing.returncode == 0
        ):
            return cmd
    fail(
        "no LHA reader found; install lhasa or pass --lha-reader. A reader is "
        "required for the control extraction."
    )



PERMISSION_LINE = re.compile(r"^[-dl][rwxstST-]{9}\s")


def expected_archive_paths():
    paths = [target for _source, target in DOCUMENTS]
    paths += [target for _template, target in ICONS]
    paths += ["Raptor/%s" % name for name in BINARY_NAMES]
    return sorted(paths)


def archive_names(reader: str, archive: Path):
    proc = run([reader, "l", str(archive)], check=False)
    if proc is None or proc.returncode != 0:
        fail("cannot list archive with %s" % reader)
    names = []
    for line in proc.stdout.splitlines():
        line = line.strip()
        if not PERMISSION_LINE.match(line):
            continue
        permission = line.split()[0]
        if not permission.strip("-"):
            continue
        names.append(line.split()[-1])
    return names


def verify_archive(archive: Path, staged: Path, writer: str, writer_options, reader: str, workdir: Path):
    data = archive.read_bytes()
    if len(data) < 22:
        fail("archive is too small to be a valid LHA file")
    method = data[2:7].decode("ascii", "replace")
    level = data[20]
    if method != "-lh5-":
        fail("archive method is %r, expected '-lh5-'" % method)
    if level != 2:
        fail("archive header level is %d, expected 2" % level)

    writer_test = run([writer, "t", str(archive)], check=False)
    if writer_test is None or writer_test.returncode != 0:
        fail("writer integrity test failed for %s" % archive)
    reader_test = run([reader, "t", str(archive)], check=False)
    if reader_test is None or reader_test.returncode != 0:
        fail("independent reader integrity test failed for %s" % archive)

    names = archive_names(reader, archive)
    expected = expected_archive_paths()
    if names != expected:
        missing = [name for name in expected if name not in names]
        extra = [name for name in names if name not in expected]
        if not missing and not extra:
            detail = "the same entries are present but the order differs"
        else:
            detail = "missing: %s; extra: %s" % (
                ", ".join(missing) or "none",
                ", ".join(extra) or "none",
            )
        fail("archive listing differs from the verified layout; %s" % detail)

    extract_dir = Path(tempfile.mkdtemp(prefix="raptor-extract-", dir=str(workdir)))
    try:
        extraction = run([reader, "xq", str(archive)], cwd=extract_dir, check=False)
        if extraction is None or extraction.returncode != 0:
            fail("control extraction failed for %s" % archive)
        found = []
        for path in extract_dir.rglob("*"):
            if path.is_symlink():
                fail("control extraction produced a symlink: %s" % path)
            if path.is_file():
                found.append(path.relative_to(extract_dir).as_posix())
        found.sort()
        if found != expected:
            fail(
                "control extraction produced %d files, expected %d; listing: %s"
                % (len(found), len(expected), ", ".join(found))
            )
        for name in expected:
            extracted = extract_dir / name
            original = staged / name
            if extracted.stat().st_size != original.stat().st_size:
                fail("size mismatch after control extraction: %s" % name)
            if sha256_file(extracted) != sha256_file(original):
                fail("content mismatch after control extraction: %s" % name)
    finally:
        shutil.rmtree(extract_dir, ignore_errors=True)

    entries = []
    for name in expected:
        source = staged / name
        entries.append(
            {"name": name, "size": source.stat().st_size, "sha256": sha256_file(source)}
        )
    return {
        "method": method,
        "header_level": level,
        "size": len(data),
        "sha256": sha256_bytes(data),
        "entries": entries,
        "uncompressed_total": sum(entry["size"] for entry in entries),
        "integrity_writer": True,
        "integrity_reader": True,
        "control_extraction": True,
    }



def physical_binary_checks(bin_dir: Path, objdump: str):
    checks = {}
    details = {}
    for name in BINARY_NAMES:
        path = bin_dir / name
        if not path.is_file():
            fail("missing binary: %s" % path)
        if not is_amiga_executable(path):
            fail("%s is not an AmigaOS loadseg()ble executable" % path)
        counts = instruction_counts(path, objdump)
        validate_instruction_checks(name, counts)
        checks[name] = counts
        details[name] = {
            "name": name,
            "size": path.stat().st_size,
            "sha256": sha256_file(path),
            "checks": counts,
        }
    return checks, details


def verify_build_record(record_path: Path, version: str, source_ref: str, bin_dir: Path, repo: Path):
    record = read_json(record_path)
    if record.get("schema") != SCHEMA:
        fail("unsupported build record schema in %s" % record_path)
    if record.get("version") != version:
        fail(
            "build record version %r does not match --version %r"
            % (record.get("version"), version)
        )
    if record.get("source_ref") != source_ref:
        fail(
            "build record was created for %s, not for --source-ref %s; rebuild or "
            "re-record the binaries for the release commit"
            % (record.get("source_ref"), source_ref)
        )
    mode = record.get("mode")
    if mode not in ("built", "reused"):
        fail("build record has an unknown mode: %r" % mode)
    entries = {}
    for entry in record.get("binaries") or []:
        if isinstance(entry, dict) and entry.get("name"):
            entries[entry["name"]] = entry
    if set(entries) != set(BINARY_NAMES):
        fail("build record must describe exactly these binaries: %s" % ", ".join(BINARY_NAMES))
    for name in BINARY_NAMES:
        entry = entries[name]
        path = bin_dir / name
        if not path.is_file():
            fail("missing binary: %s" % path)
        if entry.get("sha256") != sha256_file(path):
            fail("SHA-256 mismatch for %s; the binary is not the recorded one" % name)
        if int(entry.get("size", -1)) != path.stat().st_size:
            fail("size mismatch for %s" % name)
    binaries_ref = record.get("binaries_ref") or source_ref
    resolve_commit(repo, binaries_ref)
    if mode == "built":
        worktree = record.get("worktree") or {}
        if not worktree.get("clean_before") or not worktree.get("clean_after"):
            fail("build record does not prove a clean source tree before and after the build")
        if binaries_ref != source_ref:
            fail("a fresh build record must have binaries_ref equal to source_ref")
    else:
        provenance = record.get("provenance") or {}
        if not provenance.get("previous_release") or not provenance.get("previous_manifest_sha256"):
            fail("reused build record does not preserve the previous release provenance")
    return record, binaries_ref, mode



def stage_release(repo: Path, source_ref: str, bin_dir: Path, icons_dir: Path, workdir: Path) -> Path:
    staging = Path(tempfile.mkdtemp(prefix="raptor-stage-", dir=str(workdir)))
    for name in BINARY_NAMES:
        source = bin_dir / name
        target = staging / "Raptor" / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        target.chmod(0o755)
    for source_path, target_path in DOCUMENTS:
        data = git_show_bytes(repo, source_ref, source_path)
        if not data:
            fail("document %s is empty at %s" % (source_path, source_ref))
        target = staging / target_path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        target.chmod(0o644)
    for template, target_path in ICONS:
        source = icons_dir / template
        if not source.is_file():
            fail("missing icon template: %s" % source)
        target = staging / target_path
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        target.chmod(0o755)
    found = sorted(
        path.relative_to(staging).as_posix() for path in staging.rglob("*") if path.is_file()
    )
    if found != expected_archive_paths():
        fail("staged release set differs from the verified layout: %s" % ", ".join(found))
    return staging


def release_file_names(version: str):
    return [
        "raptor.lha",
        "raptor.readme",
        "RELEASE_NOTES_%s.md" % version,
        "MANIFEST.txt",
        "SHA256SUMS.txt",
    ]


def write_checksums(directory: Path, names) -> None:
    lines = ["%s  %s" % (sha256_file(directory / name), name) for name in names]
    write_text(directory / "SHA256SUMS.txt", "\n".join(lines) + "\n")


def verify_checksums(directory: Path) -> None:
    sums_path = directory / "SHA256SUMS.txt"
    for line in sums_path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            fail("malformed checksum line: %s" % line)
        digest, name = parts[0], parts[1].strip()
        path = directory / name
        if not path.is_file():
            fail("checksum file references a missing file: %s" % name)
        if sha256_file(path) != digest:
            fail("checksum mismatch: %s" % name)



def install_release(prepared: Path, out_dir: Path, version: str, force: bool = False, replace=os.replace):
    """Install the five release files, keeping staging and backup next to out_dir."""
    prepared = Path(prepared)
    out_dir = Path(out_dir)
    names = release_file_names(version)
    parent = out_dir.parent
    parent.mkdir(parents=True, exist_ok=True)
    existed = out_dir.exists()
    if existed and not out_dir.is_dir():
        fail("output path exists and is not a directory: %s" % out_dir)
    entries = sorted(path.name for path in out_dir.iterdir()) if existed else []
    if entries and entries != sorted(names):
        fail(
            "output directory %s contains files that are not a complete release: %s; "
            "refusing to delete or overwrite anything" % (out_dir, ", ".join(entries))
        )
    if entries == sorted(names) and not all((out_dir / name).is_file() for name in names):
        fail("output directory %s does not contain five regular release files" % out_dir)
    if entries == sorted(names) and not force:
        fail(
            "a complete release already exists in %s; pass --force to replace it "
            "explicitly" % out_dir
        )
    staging = Path(tempfile.mkdtemp(prefix=".%s.staging-" % out_dir.name, dir=str(parent)))
    backup = None
    created_out_dir = False
    try:
        for name in names:
            source = prepared / name
            if not source.is_file():
                fail("prepared release is missing %s" % name)
            shutil.copyfile(source, staging / name)
            if sha256_file(staging / name) != sha256_file(source):
                fail("internal staging copy verification failed: %s" % name)
        previous_hashes = {}
        if entries == sorted(names):
            previous_hashes = {name: sha256_file(out_dir / name) for name in names}
            backup = Path(tempfile.mkdtemp(prefix=".%s.backup-" % out_dir.name, dir=str(parent)))
            for name in names:
                shutil.copy2(out_dir / name, backup / name)
                if sha256_file(backup / name) != sha256_file(out_dir / name):
                    fail("backup verification failed: %s" % name)
        if not existed:
            out_dir.mkdir(parents=True)
            created_out_dir = True
        installed = []
        try:
            for name in names:
                replace(staging / name, out_dir / name)
                installed.append(name)
            for name in names:
                if sha256_file(out_dir / name) != sha256_file(prepared / name):
                    fail("installed file mismatch: %s" % name)
        except Exception as exc:
            had_previous = backup is not None
            rollback_problems = []
            for name in reversed(installed):
                try:
                    if backup is not None:
                        replace(backup / name, out_dir / name)
                    else:
                        (out_dir / name).unlink()
                except Exception as rollback_exc:
                    rollback_problems.append("%s: %s" % (name, rollback_exc))
            if rollback_problems:
                fail(
                    "installation failed (%s) and rollback was incomplete: %s; "
                    "backup kept at %s" % (exc, "; ".join(rollback_problems), backup)
                )
            if backup is not None:
                for name in names:
                    if sha256_file(out_dir / name) != previous_hashes[name]:
                        fail(
                            "rollback verification failed for %s; backup kept at %s"
                            % (name, backup)
                        )
                shutil.rmtree(backup, ignore_errors=True)
                backup = None
            if created_out_dir and out_dir.exists() and not any(out_dir.iterdir()):
                out_dir.rmdir()
            if not had_previous:
                fail(
                    "installation failed (%s); partial files were removed and no previous "
                    "release existed" % exc
                )
            fail("installation failed (%s); the previous release was restored" % exc)
        if backup is not None:
            shutil.rmtree(backup, ignore_errors=True)
            backup = None
    finally:
        shutil.rmtree(staging, ignore_errors=True)



def format_table(headers, rows):
    widths = [len(str(header)) for header in headers]
    for row in rows:
        for index, cell in enumerate(row):
            widths[index] = max(widths[index], len(str(cell)))
    lines = [
        "  ".join(str(header).ljust(widths[index]) for index, header in enumerate(headers)).rstrip(),
        "  ".join("-" * width for width in widths).rstrip(),
    ]
    for row in rows:
        lines.append(
            "  ".join(str(cell).ljust(widths[index]) for index, cell in enumerate(row)).rstrip()
        )
    return "\n".join(lines)


def build_manifest(ctx) -> str:
    lines = []
    lines.append("Raptor Amiga Port %s - release manifest" % ctx["version"])
    lines.append("=" * (len(lines[0])))
    lines.append("")
    lines.append("Prepared: %s" % ctx["prepared"])
    lines.append("Release date (CHANGELOG): %s" % ctx["release_date"])
    lines.append("Repository: %s (git)" % ctx["repository_name"])
    lines.append("")
    lines.append("Source revision")
    lines.append("---------------")
    lines.append("Source commit: %s" % ctx["source_ref"])
    tag = ctx.get("tag")
    if tag:
        lines.append(
            "Tag: %s (%s object %s) -> %s"
            % (tag["name"], tag["type"], tag["object"], tag["commit"])
        )
    else:
        lines.append(
            "Tag: none at packaging time; this is a candidate package for commit %s."
            % ctx["source_ref"]
        )
        lines.append(
            "     Publication must create the tag %s on that exact commit." % ctx["version"]
        )
    lines.append("Binaries built from: %s" % ctx["binaries_ref"])
    lines.append("Build record: mode=%s sha256=%s" % (ctx["mode"], ctx["record_sha256"]))
    changes = ctx.get("reuse_changes") or []
    if ctx["binaries_ref"] == ctx["source_ref"]:
        lines.append("Reuse: not applicable (binaries and sources share the same commit).")
    elif changes:
        lines.append("Reuse: allowed; only non-build paths changed between the two commits:")
        for status, paths in changes:
            lines.append("  %s %s" % (status, " ".join(paths)))
    else:
        lines.append("Reuse: no file changes between the binary commit and the source commit.")
    lines.append("")
    lines.append("Binaries")
    lines.append("--------")
    if ctx["mode"] == "built":
        lines.append(
            "The four binaries were built by tools/build_amiga_release.py from the source"
        )
        lines.append("commit above, after a clean-tree check before and after each build.")
    else:
        lines.append(
            "The game was NOT recompiled for this package. The four binaries are the"
        )
        lines.append(
            "verified binaries of the previous release; their provenance is preserved in"
        )
        lines.append(
            "the previous release manifest identified below. The current toolchain was used"
        )
        lines.append("only to verify them.")
    provenance = ctx["build_record"].get("provenance") or {}
    if provenance:
        lines.append(
            "Previous release: %s (manifest %s, archive %s)"
            % (
                provenance.get("previous_release", "?"),
                provenance.get("previous_manifest_sha256", "?"),
                provenance.get("previous_archive_sha256", "?"),
            )
        )
        if provenance.get("previous_commit"):
            lines.append("Previous package commit: %s" % provenance["previous_commit"])
    toolchain = ctx["build_record"].get("toolchain") or {}
    if toolchain.get("built_with"):
        lines.append("Toolchain that built them: %s" % toolchain["built_with"])
    verified_with = toolchain.get("verified_with")
    if isinstance(verified_with, dict):
        lines.append(
            "Verified with: %s"
            % ", ".join("%s (%s)" % (key, value) for key, value in sorted(verified_with.items()))
        )
    elif verified_with:
        lines.append("Verified with: %s" % verified_with)
    rows = []
    for name, target in BINARIES:
        detail = ctx["bin_details"][name]
        counts = detail["checks"]
        rows.append(
            (
                name,
                detail["size"],
                detail["sha256"],
                target,
                "%d/%d" % (counts["fpu_instructions"], counts["m040_only_instructions"]),
            )
        )
    lines.append(
        format_table(("name", "size", "SHA-256", "target", "FPU/68040-only insns"), rows)
    )
    lines.append("")
    lines.append("Icons")
    lines.append("-----")
    icon_source = ctx["icon_record"].get("source") or {}
    verification = ctx["icon_record"].get("verification") or {}
    lines.append(
        "Origin: %s" % (icon_source.get("note") or "packaging/amiga/icons")
    )
    lines.append(
        "Import verification: %s %s, %s"
        % (
            verification.get("tool", "amigainfo"),
            verification.get("tool_version", "?"),
            verification.get("what_was_checked", ""),
        )
    )
    rows = []
    by_file = {entry["file"]: entry for entry in ctx["icon_record"].get("icons", [])}
    for template, target in ICONS:
        entry = by_file.get(template, {})
        rows.append(
            (
                target,
                entry.get("type", "?"),
                entry.get("size", "?"),
                entry.get("sha256", "?"),
                len(entry.get("tooltypes") or []),
            )
        )
    lines.append(format_table(("archive path", "type", "size", "SHA-256", "ToolTypes"), rows))
    lines.append("")
    lines.append("Documents (read from commit %s, not from the working tree)" % ctx["source_ref"])
    lines.append("-" * len(lines[-1]))
    rows = [
        (entry["source"], entry["target"], entry["size"], entry["sha256"])
        for entry in ctx["documents"]
    ]
    lines.append(format_table(("repository path", "archive path", "size", "SHA-256"), rows))
    lines.append("")
    lines.append("Tooling and templates used")
    lines.append("--------------------------")
    rows = []
    for entry in ctx["tooling"]:
        state = "external"
        if not entry["external"]:
            state = "dirty" if entry["dirty"] else "HEAD"
        if entry["commit"]:
            state = entry["commit"]
        rows.append((entry["file"], state, entry["sha256"]))
    lines.append(format_table(("file", "commit / state", "SHA-256"), rows))
    lines.append("")
    lines.append("Archive")
    lines.append("-------")
    archive = ctx["archive"]
    lines.append("File: raptor.lha")
    lines.append("Size: %d bytes" % archive["size"])
    lines.append("SHA-256: %s" % archive["sha256"])
    lines.append("Method: %s; header level: %d" % (archive["method"], archive["header_level"]))
    lines.append("Writer: %s" % ctx["writer_label"])
    lines.append("Reader: %s" % ctx["reader_label"])
    lines.append(
        "Entries: %d; uncompressed total: %d bytes"
        % (len(archive["entries"]), archive["uncompressed_total"])
    )
    lines.append("")
    rows = [
        (entry["size"], entry["sha256"], entry["name"]) for entry in archive["entries"]
    ]
    lines.append(format_table(("size", "SHA-256", "name"), rows))
    lines.append("")
    lines.append("Verification performed")
    lines.append("----------------------")
    lines.append("- Source documents were read from commit %s with git." % ctx["source_ref"])
    lines.append("- Version consistency checks passed:")
    for check in ctx["version_checks"]:
        lines.append("    %s" % check)
    lines.append(
        "- Binaries: AmigaOS HUNK magic, sizes and SHA-256 match the build record."
    )
    lines.append(
        "- Instruction checks: no FPU instructions in the soft-float builds, no"
    )
    lines.append(
        "  68040/68060-only instructions in the 68030 builds."
    )
    if ctx["binaries_ref"] != ctx["source_ref"]:
        lines.append(
            "- Automatic binary reuse accepted: every change between %s and %s is on"
            % (ctx["binaries_ref"], ctx["source_ref"])
        )
        lines.append("  the conservative non-build allow-list.")
    lines.append(
        "- Icons: pinned SHA-256 values, import verification record and inactive CD32"
    )
    lines.append("  ToolTypes checked; no active CD32 entry present.")
    lines.append(
        "- Archive integrity: writer test and independent reader test passed (CRC)."
    )
    lines.append(
        "- Archive listing: exactly %d entries in the verified order."
        % len(ctx["archive"]["entries"])
    )
    lines.append(
        "- Control extraction with the independent reader: %d/%d files byte-identical;"
        % (len(ctx["archive"]["entries"]), len(ctx["archive"]["entries"]))
    )
    lines.append("  no extra files and no symlinks.")
    lines.append("- Forbidden content scan: no game data, music, savegames, personal")
    lines.append("  amiga.cfg, logs, object files, tools or temporary files.")
    lines.append("- SHA256SUMS.txt verified after writing; it works when all release")
    lines.append("  files are downloaded into one directory.")
    lines.append("- Output directory contains only the five release files.")
    lines.append("")
    lines.append("Not included (by design)")
    lines.append("------------------------")
    lines.append("Original GLB game data (FILE0000.GLB..FILE0004.GLB), MP3/WAVE music,")
    lines.append("savegames, a personal amiga.cfg, .git, object files, build logs,")
    lines.append("staging and control-extraction directories.")
    lines.append("")
    lines.append("Release directory")
    lines.append("-----------------")
    for name in ctx["out_files"]:
        lines.append("  %s" % name)
    lines.append("")
    lines.append("Publication")
    lines.append("-----------")
    lines.append("No tag was created, moved or deleted; nothing was committed or pushed and")
    lines.append("nothing was published to GitHub or Aminet by this packaging run.")
    return "\n".join(lines) + "\n"



def verify_previous_release(previous: Path):
    sums_path = previous / "SHA256SUMS.txt"
    manifest_path = previous / "MANIFEST.txt"
    for path in (sums_path, manifest_path):
        if not path.is_file():
            fail("previous release is missing %s" % path)
    verify_checksums(previous)
    manifest_text = manifest_path.read_text(encoding="utf-8")
    version_match = re.search(r"^Raptor Amiga Port (.+?) - release manifest$", manifest_text, re.M)
    if not version_match:
        fail("cannot read the previous release version from %s" % manifest_path)
    commit_match = re.search(r"Source commit:\s*([0-9a-f]{40})", manifest_text)
    if not commit_match:
        commit_match = re.search(r"Commit[^\n]*:\s*\n\s*([0-9a-f]{40})", manifest_text)
    built_match = re.search(r"built at\s*\n\s*([0-9a-f]{40})", manifest_text)
    if not built_match:
        built_match = re.search(r"Binaries built from:\s*([0-9a-f]{40})", manifest_text)
    if not commit_match:
        fail("cannot read the previous source commit from %s" % manifest_path)
    binaries = {}
    for digest, name in re.findall(
        r"(?m)^\s*([0-9a-f]{64})\s+(raptor(?:_nofpu|_030|_030_fpu)?)\s", manifest_text
    ):
        binaries[name] = digest
    for name, digest in re.findall(
        r"(?m)^\s*(raptor(?:_nofpu|_030|_030_fpu)?)\s+\d+\s+([0-9a-f]{64})\s",
        manifest_text,
    ):
        binaries.setdefault(name, digest)
    if set(binaries) != set(BINARY_NAMES):
        fail(
            "previous manifest does not list the four binaries; found: %s"
            % (", ".join(sorted(binaries)) or "none")
        )
    archive_sums = {}
    for line in sums_path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            digest, name = line.split(None, 1)
            archive_sums[name.strip()] = digest
    if "raptor.lha" not in archive_sums:
        fail("previous SHA256SUMS.txt does not list raptor.lha")
    return {
        "version": version_match.group(1),
        "commit": commit_match.group(1),
        "binaries_ref": built_match.group(1) if built_match else commit_match.group(1),
        "binaries": binaries,
        "manifest_sha256": sha256_file(manifest_path),
        "archive_sha256": archive_sums["raptor.lha"],
    }


def cmd_record_reuse(args):
    repo = git_root(Path(args.repo) if args.repo else Path.cwd())
    previous = Path(args.previous_release).resolve()
    info = verify_previous_release(previous)
    bin_dir = Path(args.bin_dir).resolve()
    for name in BINARY_NAMES:
        path = bin_dir / name
        if not path.is_file():
            fail("missing binary: %s" % path)
        if not is_amiga_executable(path):
            fail("%s is not an AmigaOS loadseg()ble executable" % path)
        if sha256_file(path) != info["binaries"][name]:
            fail(
                "%s does not match the verified previous release %s"
                % (name, info["version"])
            )
    objdump = objdump_tool()
    if objdump is None:
        fail("m68k-amigaos-objdump not found; cannot record instruction checks")
    checks = {}
    binaries = []
    for name in BINARY_NAMES:
        path = bin_dir / name
        counts = instruction_counts(path, objdump)
        validate_instruction_checks(name, counts)
        checks[name] = counts
        binaries.append(
            {
                "name": name,
                "size": path.stat().st_size,
                "sha256": sha256_file(path),
                "checks": counts,
            }
        )
    head = resolve_commit(repo, "HEAD")
    tag = tag_info(repo, args.version)
    status = git(repo, "status", "--porcelain")
    verified_with = {
        "m68k-amigaos-objdump": toolchain_line(objdump),
    }
    record = {
        "schema": SCHEMA,
        "mode": "reused",
        "version": args.version,
        "source_ref": head,
        "binaries_ref": info["binaries_ref"],
        "tag": tag,
        "worktree": {"clean": not bool((status or "").strip())},
        "binaries": binaries,
        "checks": checks,
        "provenance": {
            "previous_release": info["version"],
            "previous_release_dir": previous.name,
            "previous_manifest_sha256": info["manifest_sha256"],
            "previous_archive_sha256": info["archive_sha256"],
            "previous_commit": info["commit"],
        },
        "toolchain": {
            "built_with": "unchanged from the verified previous release; see provenance",
            "verified_with": verified_with,
        },
        "created": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    write_text(out, json.dumps(record, indent=2, sort_keys=False) + "\n")
    print("build record: %s" % out)
    print("mode: reused (no compilation)")
    print("binaries verified against: %s" % previous)
    for name in BINARY_NAMES:
        print("  %s %s" % (sha256_file(bin_dir / name), name))
    return 0



def render_descriptions(
    templates_dir: Path,
    version: str,
    previous: str,
    info,
    changelog_body: str,
    source_reference: str,
    destination: Path,
    notes_name: str,
):
    uploader = (info.get("uploader") or "").strip()
    if not uploader:
        fail("missing 'uploader' in --release-info (or pass --uploader)")
    summary = (info.get("summary") or "").strip()
    if not summary:
        summary = (
            "This release replaces %s. The archive is distributed as raptor.lha so "
            "future Aminet updates can replace the same archive name." % previous
        )
    whats_new = (info.get("whats_new_md") or changelog_body).strip()
    upgrade = (info.get("upgrade_notes_md") or "").strip()
    testing = (info.get("testing_status_md") or "").strip()
    if not testing:
        testing = "No new hardware testing is claimed for this release."

    readme_template = (templates_dir / "raptor.readme.in").read_text(encoding="utf-8")
    notes_template = (templates_dir / "RELEASE_NOTES.md.in").read_text(encoding="utf-8")

    readme_values = {
        "VERSION": version,
        "UPLOADER": uploader,
        "SUMMARY": wrap_paragraphs(summary),
        "WHATS_NEW": normalize_text(markdown_to_text(whats_new)),
        "UPGRADE_NOTES": (
            "Upgrading:\n\n" + normalize_text(markdown_to_text(upgrade)) if upgrade else ""
        ),
        "TESTING_STATUS": normalize_text(markdown_to_text(testing)),
    }
    notes_values = {
        "VERSION": version,
        "SUMMARY": summary,
        "WHATS_NEW_MD": whats_new,
        "UPGRADE_SECTION_MD": ("## Upgrading\n\n" + upgrade if upgrade else ""),
        "TESTING_SECTION_MD": "## Testing status\n\n" + testing,
        "SOURCE_URL": source_reference,
    }
    write_text(destination / "raptor.readme", render_template(readme_template, readme_values))
    write_text(destination / notes_name, render_template(notes_template, notes_values))



def cmd_package(args):
    repo = git_root(Path(args.repo) if args.repo else Path.cwd())
    version = args.version
    source_ref = resolve_commit(repo, args.source_ref)

    tag = tag_info(repo, args.tag or version)
    if args.tag and tag is None:
        fail(
            "tag %s does not exist; packaging a candidate does not require a tag, "
            "but an explicitly requested tag must exist" % args.tag
        )
    if tag is not None and tag["commit"] != source_ref:
        fail(
            "tag %s points to %s, not to --source-ref %s; the script never moves tags"
            % (tag["name"], tag["commit"], source_ref)
        )

    bin_dir = Path(args.bin_dir).resolve()
    icons_dir = (
        Path(args.icons_dir).resolve() if args.icons_dir else repo / "packaging/amiga/icons"
    )
    templates_dir = (
        Path(args.templates_dir).resolve()
        if args.templates_dir
        else repo / "packaging/amiga/templates"
    )
    release_info_path = Path(args.release_info).resolve() if args.release_info else None
    info = load_release_info(release_info_path)
    if args.uploader:
        info["uploader"] = args.uploader

    release_date, changelog_body, version_checks = check_version_at_ref(
        repo, source_ref, version
    )
    record_path = Path(args.build_record).resolve()
    build_record, binaries_ref, mode = verify_build_record(
        record_path, version, source_ref, bin_dir, repo
    )
    reuse_changes = check_reuse_allowed(repo, binaries_ref, source_ref)

    objdump = objdump_tool()
    if objdump is None:
        fail("m68k-amigaos-objdump not found; the release toolchain is incomplete")
    current_checks, bin_details = physical_binary_checks(bin_dir, objdump)
    recorded_checks = build_record.get("checks") or {}
    for name in BINARY_NAMES:
        if recorded_checks.get(name) and recorded_checks[name] != current_checks[name]:
            fail("instruction checks recorded for %s no longer match the binary" % name)

    icon_record = load_icon_verification(icons_dir)
    previous = info.get("previous_version") or derive_previous_version(repo, version)
    workdir = Path(tempfile.mkdtemp(prefix="raptor-release-"))
    keep = bool(args.keep_staging)
    try:
        staging = stage_release(repo, source_ref, bin_dir, icons_dir, workdir)
        prepared = Path(tempfile.mkdtemp(prefix="raptor-prepared-", dir=str(workdir)))

        writer, writer_options = find_writer(args.lha_writer)
        writer_label = "%s (%s), options: %s" % (
            Path(writer).name,
            tool_version(writer),
            " ".join(writer_options) or "default method",
        )
        archive = workdir / "raptor.lha"
        files = expected_archive_paths()
        creation = run(
            [writer, "aq", *writer_options, str(archive), *files],
            cwd=staging,
            check=False,
        )
        if creation is None or creation.returncode != 0 or not archive.is_file():
            fail("LHA writer failed to create %s" % archive)
        reader = find_reader(args.lha_reader, archive)
        reader_label = "%s (%s)" % (Path(reader).name, tool_version(reader))
        archive_info = verify_archive(archive, staging, writer, writer_options, reader, workdir)
        shutil.copyfile(archive, prepared / "raptor.lha")

        tag_name = tag["name"] if tag else None
        render_descriptions(
            templates_dir=templates_dir,
            version=version,
            previous=previous,
            info=info,
            changelog_body=changelog_body,
            source_reference=source_url(repo, tag_name, source_ref),
            destination=prepared,
            notes_name="RELEASE_NOTES_%s.md" % version,
        )


        documents = []
        for source_path, target_path in DOCUMENTS:
            path = staging / target_path
            documents.append(
                {
                    "source": source_path,
                    "target": target_path,
                    "size": path.stat().st_size,
                    "sha256": sha256_file(path),
                }
            )

        tool_paths = [Path(__file__).resolve()]
        wrapper_path = repo / "tools/build_amiga_release.py"
        if wrapper_path.is_file():
            tool_paths.append(wrapper_path)
        tool_paths += sorted(path for path in templates_dir.rglob("*") if path.is_file())
        tool_paths += sorted(path for path in icons_dir.rglob("*") if path.is_file())
        if release_info_path:
            tool_paths.append(release_info_path)
        seen = set()
        tooling = []
        for path in tool_paths:
            resolved = path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            tooling.append(file_provenance(repo, resolved))

        manifest_ctx = {
            "version": version,
            "prepared": datetime.date.today().isoformat(),
            "release_date": release_date,
            "repository_name": repo.name,
            "source_ref": source_ref,
            "tag": tag,
            "binaries_ref": binaries_ref,
            "record_sha256": sha256_file(record_path),
            "mode": mode,
            "build_record": build_record,
            "bin_details": bin_details,
            "reuse_changes": reuse_changes,
            "icon_record": icon_record,
            "documents": documents,
            "tooling": tooling,
            "archive": archive_info,
            "writer_label": writer_label,
            "reader_label": reader_label,
            "version_checks": version_checks,
            "out_files": release_file_names(version),
        }
        write_text(prepared / "MANIFEST.txt", build_manifest(manifest_ctx))
        write_checksums(
            prepared,
            [
                "raptor.lha",
                "raptor.readme",
                "RELEASE_NOTES_%s.md" % version,
                "MANIFEST.txt",
            ],
        )
        verify_checksums(prepared)
        prepared_files = sorted(path.name for path in prepared.iterdir() if path.is_file())
        if prepared_files != sorted(release_file_names(version)):
            fail("prepared release set is incomplete: %s" % ", ".join(prepared_files))

        install_release(prepared, Path(args.out_dir), version, force=args.force)

        out_dir = Path(args.out_dir).resolve()
        print("release directory: %s" % out_dir)
        for name in release_file_names(version):
            path = out_dir / name
            print("  %-28s %10d  %s" % (name, path.stat().st_size, sha256_file(path)))
        print(
            "archive verification: integrity OK, %d entries, control extraction OK"
            % len(archive_info["entries"])
        )
        print(
            "tag: %s"
            % (("%s -> %s" % (tag["name"], tag["commit"])) if tag else "none (candidate)")
        )
        print("no tag, commit, push or publication was performed")
    finally:
        if keep:
            print("staging kept at: %s" % workdir)
        else:
            shutil.rmtree(workdir, ignore_errors=True)
    return 0



def build_parser():
    parser = argparse.ArgumentParser(
        prog="package_amiga_release.py",
        description="Prepare a Raptor Amiga release package without publishing it.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    reuse = sub.add_parser(
        "record-reuse",
        help="record provenance for reusing already verified binaries",
    )
    reuse.add_argument("--version", required=True)
    reuse.add_argument("--bin-dir", required=True)
    reuse.add_argument("--previous-release", required=True)
    reuse.add_argument("--out", required=True)
    reuse.add_argument("--repo", help="repository root (default: current directory)")
    reuse.set_defaults(func=cmd_record_reuse)

    package = sub.add_parser(
        "package",
        help="build, verify and install a release set",
    )
    package.add_argument("--version", required=True)
    package.add_argument("--source-ref", required=True, help="exact commit/ref for documents")
    package.add_argument("--bin-dir", required=True)
    package.add_argument("--build-record", required=True)
    package.add_argument("--out-dir", required=True)
    package.add_argument("--release-info")
    package.add_argument("--uploader")
    package.add_argument("--lha-writer")
    package.add_argument("--lha-reader")
    package.add_argument("--icons-dir")
    package.add_argument("--templates-dir")
    package.add_argument("--tag", help="tag name to verify (default: --version)")
    package.add_argument(
        "--force",
        action="store_true",
        help="replace an existing complete release; never deletes unknown files",
    )
    package.add_argument("--keep-staging", action="store_true")
    package.add_argument("--repo", help="repository root (default: current directory)")
    package.set_defaults(func=cmd_package)
    return parser


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except ReleaseError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2
    except BrokenPipeError:
        return 1


if __name__ == "__main__":
    sys.exit(main())
