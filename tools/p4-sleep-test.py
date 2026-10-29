"""Exercise real light-sleep cycles through the P4 UART test command."""
import argparse
import re
import sys
import time
from pathlib import Path
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port', default='COM3')
parser.add_argument('--cycles', type=int, default=3)
parser.add_argument('--sleep-ms', type=int, default=2000)
parser.add_argument('--reset', action='store_true', help='Reset first and wait for game initialization')
parser.add_argument('--log', default='build/p4-sleep-test.log')
args = parser.parse_args()
if not 1 <= args.cycles <= 10:
    parser.error('--cycles must be between 1 and 10')
if not 100 <= args.sleep_ms <= 30000:
    parser.error('--sleep-ms must be between 100 and 30000')
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
log_path = Path(args.log)
log_path.parent.mkdir(parents=True, exist_ok=True)
port = serial.Serial(baudrate=921600, timeout=0.2)
port.dtr = False
port.rts = False
port.port = args.port
with port, log_path.open('w', encoding='utf-8') as log:
    def read_line():
        line = port.readline().decode('utf-8', errors='replace').strip()
        if line and '\x00' not in line:
            print(line, flush=True)
            log.write(line + '\n')
            log.flush()
            return line
        return ''
    if args.reset:
        port.rts = True
        time.sleep(0.15)
        port.rts = False
        deadline = time.monotonic() + 25
        ready = False
        while time.monotonic() < deadline:
            if 'PAL GAME READY' in read_line():
                ready = True
                break
        if not ready:
            raise SystemExit('FAIL: game initialization did not complete')
    for cycle in range(args.cycles):
        time.sleep(0.3)
        port.write(f'\nP4SLEEPTEST {args.sleep_ms}\n'.encode('ascii'))
        port.flush()
        deadline = time.monotonic() + args.sleep_ms / 1000 + 12
        resumed = False
        while time.monotonic() < deadline:
            line = read_line()
            if 'P4 sleep rejected' in line or 'P4 audio resume failed' in line:
                raise SystemExit('FAIL: sleep/audio request failed')
            if 'codec suspend=' in line and 'codec suspend=ESP_OK' not in line:
                raise SystemExit('FAIL: codec did not enter verified suspend state')
            if 'P4 sleep: resumed' in line:
                delta = re.search(r'game_delta=(\d+) ms', line)
                if ('result=ESP_OK cause=4 ' not in line or 'PSRAM=OK' not in line or
                        not delta or int(delta.group(1)) > 5):
                    raise SystemExit('FAIL: sleep retention or game-clock check failed')
                resumed = True
                break
            if 'Rebooting' in line or 'Guru Meditation' in line or 'P4 boot' in line:
                raise SystemExit('FAIL: device restarted during sleep test')
        if not resumed:
            raise SystemExit('FAIL: no successful sleep/resume report')
        print(f'PASS: sleep/resume cycle {cycle + 1}/{args.cycles}', flush=True)
print('PASS: timed light sleep, retained PSRAM and frozen game time; physical PWR wake still needs testing')
