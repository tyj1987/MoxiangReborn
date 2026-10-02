import io
import unittest
from summarize_ground_measurements import analyze

HEADER = 'schema,groundValid,groundCollider,bodyMeshCount,boundsValid,renderBoundsGap,footMarkerValid,footMarkerGap,reason\n'


class MeasurementSummaryTest(unittest.TestCase):
    def test_misses_do_not_bias_mean_or_become_zero(self):
        result = analyze(io.StringIO(HEADER +
            'npc-ground-v2,true,floor#1,2,true,-0.2,true,0.01,markers\n' +
            'npc-ground-v2,false,,0,false,NaN,false,NaN,no-reviewed-floor-hit\n'))
        self.assertEqual(result['groundInvalid'], 1)
        self.assertEqual(result['renderBoundsGap']['mean'], -0.2)
        self.assertEqual(result['footMarkerGap']['mean'], 0.01)
        self.assertIsNone(result['acceptancePassed'])

    def test_bounds_only_never_count_as_feet(self):
        result = analyze(io.StringIO(HEADER + 'npc-ground-v2,true,floor#1,3,true,-1.7,false,NaN,bounds-only\n'))
        self.assertEqual(result['footMarkerGap']['valid'], 0)
        self.assertIsNone(result['footMarkerGap']['p95'])

    def test_nearest_rank_percentile_uses_valid_samples(self):
        rows = ''.join(f'npc-ground-v2,true,floor#1,1,true,{n},false,NaN,bounds-only\n' for n in range(1, 21))
        self.assertEqual(analyze(io.StringIO(HEADER + rows))['renderBoundsGap']['p95'], 19)

    def test_rejects_inconsistent_evidence(self):
        for row in [
            'npc-ground-v2,false,,0,false,0,false,NaN,miss',
            'npc-ground-v2,true,floor#1,1,true,NaN,false,NaN,bounds-only',
            'npc-ground-v2,true,,1,true,1,false,NaN,bounds-only',
            'npc-ground-v2,false,,1,true,1,false,NaN,miss',
            'npc-ground-v2,true,floor#1,0,true,1,false,NaN,bounds-only',
            'npc-ground-v2,true,floor#1,1,true,1,false,0,bounds-only',
        ]:
            with self.subTest(row=row), self.assertRaises(ValueError):
                analyze(io.StringIO(HEADER + row + '\n'))

    def test_rejects_legacy_feet_gap(self):
        with self.assertRaises(ValueError):
            analyze(io.StringIO('elapsed,ground,feet_gap\n1,true,-1.7\n'))


if __name__ == '__main__':
    unittest.main()
