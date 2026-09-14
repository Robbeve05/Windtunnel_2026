"""WT1 client for one FreeRTOS controller. Standard library + optional pyserial."""
import argparse
from dataclasses import dataclass
import math
import secrets
import time


def number(value):
    result = float(value)
    if not math.isfinite(result):
        raise ValueError("Non-finite measurement")
    return result


def flag(value):
    if value not in ("0", "1"):
        raise ValueError("Invalid boolean")
    return value == "1"


@dataclass(frozen=True)
class Sample:
    sampled_ms: int
    state: int
    fault: int
    output_percent: float
    valid: bool
    door_closed: bool
    estop_ok: bool
    air_before_mps: float
    air_after_mps: float
    lift_n: float
    drag_n: float
    rpm_valid: bool
    fan_rpm: float


def parse_sample(line):
    fields = line.strip().split(",")
    if len(fields) != 15 or fields[:2] != ["WT1", "S"]:
        raise ValueError("Invalid WT1 sample")
    sample = Sample(int(fields[2]), int(fields[3]), int(fields[4]),
                    number(fields[5]), flag(fields[6]), flag(fields[7]), flag(fields[8]),
                    *(number(v) for v in fields[9:13]), flag(fields[13]), number(fields[14]))
    if not 0 <= sample.sampled_ms <= 0xFFFFFFFF or sample.state not in range(3) or sample.fault not in range(7):
        raise ValueError("Invalid status")
    if not 0 <= sample.output_percent <= 30 or min(sample.air_before_mps, sample.air_after_mps) < 0:
        raise ValueError("Invalid range")
    if sample.rpm_valid and sample.fan_rpm < 0:
        raise ValueError("Invalid RPM")
    return sample


class Link:
    """Single caller only. Transport: pyserial Serial, with bounded timeouts."""
    def __init__(self, port):
        self.port = port
        self.latest = None
        self.latest_received = None

    def _line(self):
        raw = self.port.read_until(b"\n", 256)
        if not raw or not raw.endswith(b"\n") or len(raw) >= 256:
            raise TimeoutError("Missing, incomplete or oversized controller response")
        return raw.decode("ascii").strip()

    def _sample(self, line):
        self.latest = parse_sample(line)
        self.latest_received = time.monotonic()
        return self.latest

    def sample(self):
        deadline = time.monotonic() + .5
        while time.monotonic() < deadline:
            line = self._line()
            if line.startswith("WT1,S,"):
                return self._sample(line)
            if line.startswith("WT1,E,"):
                raise ValueError(line)
        raise TimeoutError("No telemetry")

    def command(self, operation, percent=None):
        if operation not in ("STOP", "RESET", "MANUAL"):
            raise ValueError("Unknown operation")
        request_id = secrets.randbits(32)
        tail = operation
        if operation == "MANUAL":
            value = number(percent)
            if not 0 <= value <= 30:
                raise ValueError("Manual output must be 0..30 percent")
            tail += f",{value:.3f}"
        self.port.write(f"WT1,C,{request_id},{tail}\n".encode("ascii"))
        deadline = time.monotonic() + .5
        while time.monotonic() < deadline:
            line = self._line()
            if line.startswith("WT1,S,"):
                self._sample(line)
                continue
            parts = line.split(",")
            if len(parts) != 6 or parts[:2] != ["WT1", "A"]:
                raise ValueError("Invalid acknowledgement: " + line)
            if int(parts[2]) != request_id:
                raise ValueError("Acknowledgement ID does not match")
            if parts[3] != "1":
                raise RuntimeError("Controller rejected command; inspect telemetry and hardware readiness")
            actual = number(parts[5])
            expected = float(f"{value:.3f}") if operation == "MANUAL" else 0.0
            state = int(parts[4])
            if actual != expected or state not in (0, 1, 2) or (expected > 0 and state != 1):
                raise ValueError("Acknowledged output/state does not match command")
            return
        raise TimeoutError("No acknowledgement")


def main():
    parser = argparse.ArgumentParser(description="Windtunnel WT1 serial monitor; default output stays off")
    parser.add_argument("--port", required=True, help="/dev/serial/by-id/... or COM5")
    parser.add_argument("--manual", type=float, help="Test output 0..30%%; repeated every 200 ms")
    parser.add_argument("--reset", action="store_true", help="Explicitly clear a resolved controller fault")
    args = parser.parse_args()
    if args.manual is not None and (not math.isfinite(args.manual) or not 0 <= args.manual <= 30):
        parser.error("--manual must be finite and within 0..30")
    import serial
    with serial.Serial(args.port, 115200, timeout=.2, write_timeout=.2) as port:
        link = Link(port)
        try:
            time.sleep(2)  # Due may reset when the programming port opens.
            port.reset_input_buffer()
            link.command("STOP")
            if args.reset:
                link.command("RESET")
            last_command = 0.0
            while True:
                sample = link.sample()
                print(sample, flush=True)
                if args.manual is not None:
                    if sample.state == 2 or not (sample.valid and sample.door_closed and sample.estop_ok):
                        raise RuntimeError("Controller/sensors are not ready")
                    if time.monotonic() - last_command >= .2:
                        link.command("MANUAL", args.manual)
                        last_command = time.monotonic()
        except KeyboardInterrupt:
            pass
        finally:
            # Also runs on parse, sensor, serial and acknowledgement errors.
            # Device watchdog remains responsible if this transmission fails.
            link.command("STOP")


if __name__ == "__main__":
    main()
