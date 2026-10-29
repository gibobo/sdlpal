"""Retrieve the P4 SD power diagnostic log over UART without resetting it."""
import argparse
from pathlib import Path
import sys
import time
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port', default='COM3')
parser.add_argument('--output', default='build/p4-power-log.txt')
args = parser.parse_args()
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
port = serial.Serial(baudrate=921600, timeout=0.5)
port.dtr = port.rts = False
port.port = args.port
with port:
    port.write(b'\nP4POWERLOG\n')
    port.flush()
    deadline = time.monotonic() + 15
    started = False
    lines = []
    while time.monotonic() < deadline:
        line = port.readline().decode('utf-8', errors='replace').strip()
        if line == 'P4 POWER LOG BEGIN':
            started = True
        elif started and line == 'P4 POWER LOG END':
            output = Path(args.output)
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text('\n'.join(lines) + '\n', encoding='utf-8')
            print(f'Saved {len(lines)} power records to {output}')
            break
        elif started:
            lines.append(line)
            print(line, flush=True)
    else:
        raise SystemExit('No complete log received; wait for game startup and retry.')
