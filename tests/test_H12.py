import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "host"))
import h12_cli as m


class TestMavlink(unittest.TestCase):
    def test_crc_heartbeat(self):
        hb = m.frame(0, m.build_heartbeat())
        self.assertEqual(hb[0], 0xFE)
        self.assertEqual(len(hb), 6 + 9 + 2)

    def test_rc_channels_length(self):
        rc = m.frame(35, m.build_rc_channels([1500] * 8))
        self.assertEqual(len(rc), 6 + 18 + 2)

    def test_parse_roundtrip(self):
        hb = m.frame(0, m.build_heartbeat())
        frames = m.analyze(hb.hex() + "\n")
        self.assertEqual(len(frames), 1)
        self.assertTrue(frames[0]["crc_ok"])
        self.assertEqual(frames[0]["msgid"], 0)


if __name__ == "__main__":
    unittest.main()
