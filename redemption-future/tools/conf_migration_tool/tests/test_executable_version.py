#!/usr/bin/env python3
import unittest

from rdp_conf_migrate import RedemptionVersion, RedemptionVersionError, NoVersion


class Test_RedemptionVersion(unittest.TestCase):
    def test_invalid_version(self):
        with self.assertRaises(RedemptionVersionError):
            RedemptionVersion("3.5")
        with self.assertRaises(RedemptionVersionError):
            RedemptionVersion("TEST")

    def test_operator_greater_than(self):
        v_3_5_9 = RedemptionVersion("3.5.9")
        v_3_5_10 = RedemptionVersion("3.5.10")
        v_3_11_9 = RedemptionVersion("3.11.9")
        assert not (v_3_5_9 > v_3_5_9)
        assert not (v_3_5_9 > v_3_5_10)
        assert v_3_5_10 > v_3_5_9
        assert v_3_11_9 > v_3_5_9
        assert not (v_3_5_9 > v_3_11_9)

        v_3_5_9c = RedemptionVersion("3.5.9c")
        assert not (v_3_5_9 > v_3_5_9c)
        assert not (v_3_5_9c > v_3_5_9c)
        assert v_3_5_9c > v_3_5_9

        v_3_5_9d = RedemptionVersion("3.5.9d")
        assert not (v_3_5_9c > v_3_5_9d)
        assert v_3_5_9d > v_3_5_9c

        assert v_3_5_9 > NoVersion
        assert v_3_5_10 > NoVersion
        assert v_3_11_9 > NoVersion
        assert v_3_5_9d > NoVersion

    def test_operator_less_than(self):
        v_3_5_9 = RedemptionVersion("3.5.9")
        v_3_5_10 = RedemptionVersion("3.5.10")
        v_3_11_9 = RedemptionVersion("3.11.9")
        assert not (v_3_5_9 < v_3_5_9)
        assert not (v_3_5_10 < v_3_5_9)
        assert v_3_5_9 < v_3_5_10
        assert not (v_3_11_9 < v_3_5_9)
        assert v_3_5_9 < v_3_11_9

        v_3_5_9c = RedemptionVersion("3.5.9c")
        assert not (v_3_5_9c < v_3_5_9)
        assert not (v_3_5_9c < v_3_5_9c)
        assert v_3_5_9 < v_3_5_9c

        v_3_5_9d = RedemptionVersion("3.5.9d")
        assert not (v_3_5_9d < v_3_5_9c)
        assert v_3_5_9c < v_3_5_9d

        assert NoVersion < v_3_5_9
        assert NoVersion < v_3_5_10
        assert NoVersion < v_3_11_9
        assert NoVersion < v_3_5_9d

    def test_operator_str(self):
        assert str(RedemptionVersion("3.5.9")) == "3.5.9"

    def test_from_file(self):
        v_from_file = RedemptionVersion.from_file(
            "./tests/fixtures/REDEMPTION_VERSION")
        assert str(v_from_file) == "9.1.17"

        with self.assertRaises(Exception):
            RedemptionVersion.from_file(
                "./tests/fixtures/REDEMPTION_VERSION_KO")
