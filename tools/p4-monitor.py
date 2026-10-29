"""Read the LCD35 UART console; optionally reset into normal boot first."""
import argparse
import sys
import time
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port', default='COM3')
parser.add_argument('--seconds', type=int, default=30)
parser.add_argument('--reset', action='store_true')
args = parser.parse_args()
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
sys.stderr.reconfigure(encoding='utf-8', errors='replace')
port = serial.Serial(baudrate=921600, timeout=0.5)
port.dtr = False
port.rts = False
port.port = args.port
with port:
    if args.reset:
        port.rts = True
        time.sleep(0.15)
        port.rts = False
    deadline = time.monotonic() + args.seconds
    while time.monotonic() < deadline:
        line = port.readline().decode('utf-8', errors='replace').strip()
        if line and '\x00' not in line:
            print(line, flush=True)
