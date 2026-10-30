from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import current_tier_models as models


class CurrentTierModelsTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.catalog = self.root / "characters.json"
        self.player = self.root / "player.json"
        self.char_pack = self.root / "char_pack"
        self.references = self.root / "references"
        self.cabs = self.root / "cab_paths.json"

        characters = [
            {"id": "EXACT", "models": {"I": "unit.hm_sw_exact_t1_pre"}},
            {"id": "READY", "models": {"I": "unit.hm_sw_ready_t1_pre"}},
            {"id": "READY_ALIAS", "models": {"I": "unit.hm_sw_ready_t1_pre"}},
            {"id": "SOURCE", "models": {"I": "unit.hm_sw_source_t1_pre"}},
            {"id": "WISP", "models": {"I": "unit.c_wisp_common_pre"}},
            {"id": "POST", "models": {"I": "unit.hm_sw_post_t1_pre"}},
        ]
        player = [{"id": character["id"], "tier": "I"} for character in characters]
        self.catalog.write_text(json.dumps({"characters": characters}))
        self.player.write_text(json.dumps({"characters": player}))

        for family in ("hm_sw_exact", "hm_sw_ready", "hm_sw_source", "c_wisp_common"):
            (self.char_pack / family).mkdir(parents=True)
        (self.char_pack / "hm_sw_exact" / "hm_sw_exact_t1_rl.glb").touch()
        (self.char_pack / "hm_sw_source" / "hm_sw_source_t1.glb").touch()
        (self.char_pack / "c_wisp_common" / "c_wisp_common_t0_rl.glb").touch()
        texture_dir = self.references / "Human Male" / "unit.hm_sw_ready_t1_pre"
        texture_dir.mkdir(parents=True)
        (texture_dir / "hm_sw_ready_t1_tex.png").touch()
        self.cabs.write_text(
            json.dumps(
                {
                    "cab-hm_sw_ready_t1_pre": "/cab/ready",
                    "cab-hm_sw_source_t1_pre": "/cab/source",
                    "cab-c_wisp_common_pre": "/cab/wisp",
                }
            )
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def audit(self):
        return models.audit_models(
            self.catalog, self.player, self.char_pack, self.cabs, self.references
        )

    def test_audit_classifies_exact_sources_tierless_and_missing(self) -> None:
        report = self.audit()
        states = {row["id"]: row["status"] for row in report["characters"]}
        self.assertEqual(states["EXACT"], "exact")
        self.assertEqual(states["READY"], "rebuild_ready")
        self.assertEqual(states["READY_ALIAS"], "rebuild_ready")
        self.assertEqual(states["SOURCE"], "source_ready")
        self.assertEqual(states["WISP"], "tierless_t0")
        self.assertEqual(states["POST"], "no_char_pack_dir")
        self.assertEqual(report["target_counts"]["rebuild_ready"], 1)

    def test_inventory_deduplicates_shared_prefabs(self) -> None:
        report = self.audit()
        payload = models.inventory_payload(report)
        self.assertEqual(
            payload["todo"],
            [
                ["hm_sw_ready", "hm_sw_ready_t1"],
                ["hm_sw_source", "hm_sw_source_t1"],
            ],
        )
        selected = models.select_targets(report, "READY_ALIAS", None, False)
        self.assertEqual([target["base"] for target in selected], ["hm_sw_ready_t1"])

    def test_mutating_batch_requires_an_explicit_scope(self) -> None:
        with self.assertRaisesRegex(ValueError, "--only, --limit, or --all"):
            models.select_targets(self.audit(), None, None, False)

    def test_workspace_roster_has_its_assigned_models(self) -> None:
        report = models.audit_models()
        unresolved = [
            (row["id"], row["tier"], row["status"])
            for row in report["characters"]
            if row["status"] not in {"exact", "tierless_t0"}
        ]
        self.assertEqual(len(report["characters"]), 670)
        self.assertEqual(unresolved, [])


if __name__ == "__main__":
    unittest.main()
