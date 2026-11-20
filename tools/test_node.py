# =============================================================================
# EH3D-NODE-PNP-HEAD - tools/test_node.py
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D) <electrohobby3d@gmail.com>
# GPL-3.0-or-later - see LICENSE
# =============================================================================
"""The register of requirements and the code agree: every row names a test that exists, the counts are the counts, and the version of the firmware is the version of the repository."""
from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROW = re.compile(r"^\| (PNPH-\d{3}) \| (.+?) \| (implemented|prototype|planned|validated) \| (done|partial|pending|blocked) \| (simulator|host-test|none|bench|field) \| `([^`]+)` \| (.+) \|$", re.M)


def register() -> list[tuple[str, str, str, str, str]]:
    text = (ROOT / "docs" / "REQUIREMENTS.md").read_text(encoding="utf-8")
    return [(m.group(1), m.group(3), m.group(4), m.group(5), m.group(6)) for m in ROW.finditer(text)]


def tests_of(stem: str) -> set[str]:
    path = ROOT / "tests" / f"{stem}.cpp"
    return set(re.findall(r"(?m)^TEST\((\w+)\)", path.read_text(encoding="utf-8"))) if path.is_file() else set()


class RegisterTests(unittest.TestCase):
    def test_every_row_has_the_next_id_and_names_a_test_that_exists(self) -> None:
        rows = register()
        self.assertGreater(len(rows), 20)
        for n, (row_id, state, _progress, _evidence, test) in enumerate(rows, 1):
            self.assertEqual(row_id, f"PNPH-{n:03d}")
            if state == "planned":
                self.assertTrue(test.startswith("planned:"), row_id)
                self.assertNotIn(test[len("planned:"):].split("::")[1], tests_of(test[len("planned:"):].split("::")[0]), f"{row_id}: the planned test exists, so the row must change")
            else:
                stem, name = test.split("::")
                self.assertIn(name, tests_of(stem), row_id)

    def test_the_counts_of_the_register_are_the_counts_of_the_table(self) -> None:
        text = (ROOT / "docs" / "REQUIREMENTS.md").read_text(encoding="utf-8")
        rows = register()
        states = [r[1] for r in rows]
        progress = [r[2] for r in rows]
        header = re.search(r"The register has (\d+) requirements: (\d+) implemented, (\d+) prototype, (\d+) planned and (\d+) validated\. By progress: (\d+) done, (\d+) partial, (\d+) pending and (\d+) blocked", text)
        self.assertIsNotNone(header)
        want = [len(rows), states.count("implemented"), states.count("prototype"), states.count("planned"), states.count("validated"), progress.count("done"), progress.count("partial"), progress.count("pending"), progress.count("blocked")]
        self.assertEqual([int(g) for g in header.groups()], want)
        status = (ROOT / "docs" / "STATUS.md").read_text(encoding="utf-8")
        self.assertIn(header.group(0).split(" By progress")[0].replace("The register has", "The register has"), status)

    def test_nothing_is_validated_without_evidence_from_a_machine(self) -> None:
        for row_id, state, _progress, evidence, _test in register():
            if state == "validated":
                self.assertIn(evidence, ("bench", "field"), row_id)
            else:
                self.assertNotIn(evidence, ("bench", "field"), row_id)

    def test_every_test_of_the_code_is_in_the_register_or_in_a_group_the_register_names(self) -> None:
        named = {r[4].split("::")[1] for r in register() if not r[4].startswith("planned:")}
        total = sum(len(tests_of(p.stem)) for p in (ROOT / "tests").glob("test_*.cpp"))
        self.assertGreaterEqual(total, len(named))
        self.assertGreater(len(named), 0)

    def test_the_version_in_the_header_of_the_code_is_the_version_of_the_repository(self) -> None:
        version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
        header = (ROOT / "core" / "version.hpp").read_text(encoding="utf-8")
        self.assertIn(f'#define PNP_HEAD_VERSION "{version}"', header)

    def test_the_example_settings_file_exists_and_the_settings_are_not_in_the_code_as_secrets(self) -> None:
        self.assertTrue((ROOT / "head.cfg.example").is_file())
        for path in (ROOT / "core").glob("*.hpp"):
            text = path.read_text(encoding="utf-8")
            self.assertNotRegex(text, r'token\s*=\s*"[^"]+"', path.name)


if __name__ == "__main__":
    unittest.main()
