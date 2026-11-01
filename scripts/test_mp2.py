#!/usr/bin/env python3
"""Build and check MP2 in the existing Docker development container."""

from pathlib import Path
import re
import subprocess
import sys


CONTAINER = sys.argv[1] if len(sys.argv) > 1 else "nachos-dev"
ROOT = Path(__file__).resolve().parents[1]
CODE = "/workspace/nachos-4.0/code"
A = "../test/mp2_a.noff"
B = "../test/mp2_b.noff"
EXIT = "../test/mp2_exit.noff"


def command(directory, *args):
    result = subprocess.run(
        ["docker", "exec", "-w", f"{CODE}/{directory}", CONTAINER, *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=30,
    )
    if result.returncode:
        raise AssertionError(result.stdout[-6000:])
    return result.stdout


def run(*args):
    output = command("userprog", "./nachos", *args)
    assert "Machine halting!" in output, output[-6000:]
    assert not re.search(r"^-(111|222|333)$", output, re.MULTILINE), output[-6000:]
    return output


def require_marker(output, marker):
    assert output.splitlines().count(str(marker)) == 1, output[-6000:]


command("userprog", "make", "nachos")
command("test", "make", "mp2_a.noff", "mp2_b.noff", "mp2_exit.noff")

require_marker(run("-e", A), 111)
print("PASS: single program exits normally")

for seed in (1, 7, 42):
    output = run("-rs", str(seed), "-e", A, "-e", B)
    assert "Not enough free physical frames." not in output, output[-6000:]
    require_marker(output, 111)
    require_marker(output, 222)
    print(f"PASS: isolated code/data and registers with timer seed {seed}")

output = run("-d", "t", "-rs", "1", "-e", A, "-e", B)
assert f"Switching from: {A} to: {B}" in output
assert f"Switching from: {B} to: {A}" in output
assert f"Finishing thread: {A}" in output
assert f"Finishing thread: {B}" in output
print("PASS: both user threads switch and finish")

machine_header = (ROOT / "nachos-4.0/code/machine/machine.h").read_text()
total_frames = int(re.search(r"const int NumPhysPages = (\d+);", machine_header)[1])
output = run("-d", "a", *[arg for _ in range(4) for arg in ("-e", EXIT)])
allocated = re.findall(r"Allocated (\d+) frames; free frames: (\d+)", output)
released = re.findall(r"Released (\d+) frames; free frames: (\d+)", output)
assert len(allocated) == len(released) == 4, output[-6000:]
assert sum(int(count) for count, _ in allocated) > total_frames
assert int(released[-1][1]) == total_frames
assert "Not enough free physical frames." not in output
print("PASS: released frames are reused and BSS is zeroed")

output = run("-d", "a", "-rs", "1", "-e", A, "-e", B, "-e", A)
assert output.count("Not enough free physical frames.") == 1
released = re.findall(r"Released (\d+) frames; free frames: (\d+)", output)
assert len(released) == 2 and int(released[-1][1]) == total_frames
print("PASS: insufficient memory rejects only the extra program")

output = run("-e", "../test/mp2_missing.noff", "-e", A)
assert "Unable to open file ../test/mp2_missing.noff" in output
require_marker(output, 111)
print("PASS: a missing executable does not stop another program")
