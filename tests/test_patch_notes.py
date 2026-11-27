import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

import patch_notes  # noqa: E402


class PatchNotesTest(unittest.TestCase):
    def test_note_is_a_dated_heading_over_bullets(self):
        self.assertEqual(
            patch_notes.render_note("v0.19.0", "2026-11-15T16:00:40Z", ["New settings window.", "Flags wave."]),
            "## v0.19.0 · 15 Nov 2026\n- New settings window.\n- Flags wave.",
        )

    def test_a_server_update_is_dated_and_owes_no_release_its_notes(self):
        note = patch_notes.server_update_note("2026-11-22T01:04:40+00:00", ["Nexus drops match their cards."])
        self.assertEqual(note, "## Server update · 22 Nov 2026\n- Nexus drops match their cards.")
        self.assertEqual(patch_notes.posted_versions([note]), set())
        self.assertEqual(patch_notes.missing_versions(["v0.29.0"], [note]), ["v0.29.0"])

    def test_note_lines_drop_bullets_and_blank_lines(self):
        self.assertEqual(patch_notes.note_lines("- one\n\n* two\n  three  \n"), ["one", "two", "three"])

    def test_previous_tag_orders_versions_by_number(self):
        tags = ["v0.9.0", "v0.10.0", "v0.12.2", "v0.13.0"]
        self.assertEqual(patch_notes.previous_tag(tags, "v0.10.0"), "v0.9.0")
        self.assertEqual(patch_notes.previous_tag(tags, "v0.13.0"), "v0.12.2")
        self.assertIsNone(patch_notes.previous_tag(tags, "v0.9.0"))

    def test_pull_requests_come_from_squash_and_merge_subjects(self):
        subjects = ["Add Renown (#57)", "Merge pull request #5 from owner/branch", "Add Renown follow-up (#57)", "Bump"]
        self.assertEqual(patch_notes.pull_request_numbers(subjects), [5, 57])

    def test_missing_versions_are_those_without_a_heading(self):
        contents = ["## v0.2.0 · 6 Nov 2026\n- b\n\n## v0.3.0 · 7 Nov 2026\n- c", "v0.4.0 is mentioned, not headed"]
        self.assertEqual(
            patch_notes.missing_versions(["v0.10.0", "v0.4.0", "v0.3.0", "v0.2.0"], contents), ["v0.4.0", "v0.10.0"]
        )


if __name__ == "__main__":
    unittest.main()
