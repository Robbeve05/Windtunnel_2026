import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "raspberry_pi"))
from windtunnel_link import Link, parse_sample

SAMPLE = b"WT1,S,100,0,0,0.000,1,1,1,4.2,3.8,-.3,.1,0,0\n"


class Port:
    def __init__(self, bad_id=False, accepted=True):
        self.lines = []
        self.bad_id = bad_id
        self.accepted = accepted

    def write(self, raw):
        fields = raw.decode().strip().split(",")
        request_id = int(fields[2]) + self.bad_id
        percent = fields[4] if fields[3] == "MANUAL" else "0"
        state = 1 if float(percent) > 0 else 0
        self.lines = [SAMPLE, f"WT1,A,{request_id},{int(self.accepted)},{state},{percent}\n".encode()]

    def read_until(self, *args):
        return self.lines.pop(0) if self.lines else b""


class TestLink(unittest.TestCase):
    def test_sample(self):
        s = parse_sample(SAMPLE.decode())
        self.assertEqual(s.lift_n, -.3)
        self.assertFalse(s.rpm_valid)

    def test_bad_sample(self):
        for line in ["WT1,S,1", SAMPLE.decode().replace("4.2", "nan"),
                     SAMPLE.decode().replace("4.2", "-1"), SAMPLE.decode().replace(",1,1,1,", ",2,1,1,")]:
            with self.assertRaises(ValueError):
                parse_sample(line)

    def test_command_with_interleaved_sample(self):
        link = Link(Port())
        link.command("MANUAL", 20)
        self.assertIsNotNone(link.latest)
        link.command("STOP")

    def test_rejected_command(self):
        with self.assertRaises(RuntimeError):
            Link(Port(accepted=False)).command("MANUAL", 20)

    def test_wrong_ack(self):
        with self.assertRaises(ValueError):
            Link(Port(bad_id=True)).command("STOP")

    def test_timeout(self):
        with self.assertRaises(TimeoutError):
            Link(Port()).sample()

    def test_invalid_setpoint(self):
        for value in [-1, 31, float("nan"), float("inf")]:
            with self.assertRaises(ValueError):
                Link(Port()).command("MANUAL", value)


if __name__ == "__main__":
    unittest.main()
