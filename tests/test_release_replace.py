#!/usr/bin/env python3
"""Tests for the safe release installation step (no network, no LHA needed)."""

from __future__ import annotations

import importlib.util
import os
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "package_amiga_release", ROOT / "tools" / "package_amiga_release.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

VERSION = "1.2.3-rc.1"


class InstallReleaseTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.prepared = self.root / "prepared"
        self.out = self.root / "release"
        self.prepared.mkdir()
        for index, name in enumerate(MODULE.release_file_names(VERSION)):
            (self.prepared / name).write_text("new-%d-%s\n" % (index, name), encoding="utf-8")

    def tearDown(self):
        self.temp.cleanup()

    def make_previous_release(self):
        self.out.mkdir()
        for index, name in enumerate(MODULE.release_file_names(VERSION)):
            (self.out / name).write_text("old-%d-%s\n" % (index, name), encoding="utf-8")

    def assert_previous_release_intact(self):
        for index, name in enumerate(MODULE.release_file_names(VERSION)):
            self.assertEqual(
                (self.out / name).read_text(encoding="utf-8"),
                "old-%d-%s\n" % (index, name),
            )
        self.assertEqual(
            sorted(path.name for path in self.root.iterdir()), ["prepared", "release"]
        )

    def test_installs_new_release_and_removes_backup(self):
        self.make_previous_release()
        MODULE.install_release(self.prepared, self.out, VERSION, force=True)
        for index, name in enumerate(MODULE.release_file_names(VERSION)):
            self.assertEqual(
                (self.out / name).read_text(encoding="utf-8"),
                "new-%d-%s\n" % (index, name),
            )
        self.assertEqual(sorted(path.name for path in self.root.iterdir()), ["prepared", "release"])

    def test_simulated_failure_rolls_back(self):
        self.make_previous_release()
        calls = {"count": 0}

        def flaky_replace(source, destination):
            calls["count"] += 1
            if calls["count"] == 3:
                raise OSError("simulated replacement failure")
            os.replace(source, destination)

        with self.assertRaises(MODULE.ReleaseError) as raised:
            MODULE.install_release(
                self.prepared, self.out, VERSION, force=True, replace=flaky_replace
            )
        self.assertIn("previous release was restored", str(raised.exception))
        self.assert_previous_release_intact()

    def test_refuses_unknown_files(self):
        self.out.mkdir()
        (self.out / "keep-me.txt").write_text("do not delete\n", encoding="utf-8")
        with self.assertRaises(MODULE.ReleaseError):
            MODULE.install_release(self.prepared, self.out, VERSION, force=True)
        self.assertEqual((self.out / "keep-me.txt").read_text(encoding="utf-8"), "do not delete\n")

    def test_requires_force_for_complete_release(self):
        self.make_previous_release()
        with self.assertRaises(MODULE.ReleaseError) as raised:
            MODULE.install_release(self.prepared, self.out, VERSION, force=False)
        self.assertIn("--force", str(raised.exception))
        self.assert_previous_release_intact()


if __name__ == "__main__":
    unittest.main()
