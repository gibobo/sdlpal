"""Deploy game data through the LCD35 backend's CRC-checked UART SD mode.

Requires pyserial. Save slots (*.rpg) are deliberately excluded.
"""
import argparse
from pathlib import Path
import time
import zlib
import sys
import json
import serial


def reset_board(port):
    # Standard Espressif DTR/RTS reset, leaving GPIO0 high for normal boot.
    port.dtr = False
    port.rts = True
    time.sleep(0.15)
    port.rts = False


def wait_line(port, prefixes, timeout=20):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        line = port.readline().decode('utf-8', errors='replace').strip()
        if not line:
            continue
        if '\x00' in line:
            continue # ROM boot output uses 115200 before the app switches baud.
        if line.startswith('ERR'):
            raise RuntimeError(line)
        if line.startswith(prefixes):
            return line
        print(line, flush=True)
    raise TimeoutError(f'No response matching {prefixes!r}')


def crc_file(path):
    crc = 0
    with path.open('rb') as source:
        while block := source.read(65536):
            crc = zlib.crc32(block, crc)
    return crc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='COM3')
    parser.add_argument('--baud', type=int, default=921600)
    parser.add_argument('--data', type=Path, default=Path('Pal98rqptw'))
    parser.add_argument('--cache', type=Path, default=Path('cache_data'))
    parser.add_argument('--music', type=Path)
    parser.add_argument('--check', action='store_true', help='list and verify files without modifying the card')
    parser.add_argument('--list-only', action='store_true', help='list card files without checks or writes')
    parser.add_argument('--report', type=Path, default=Path('build/p4-sd-report.json'))
    parser.add_argument('--monitor', type=int, default=45, help='seconds to monitor after restart')
    args = parser.parse_args()
    files = [(path.name.lower(), path) for path in sorted(args.data.glob('*.mkf'))]
    for name in ('word.bin', 'msg.bin', 'unicode_font.bin', 'unicode_font.dat'):
        files.append((name, args.cache / name))
    if args.music:
        files.extend(('mus/' + path.name, path) for path in sorted(args.music.glob('*_22050.pcm')))
    if not args.list_only and (len(files) < 19 or any(not path.is_file() for _, path in files)):
        raise RuntimeError('Missing MKF data or generated text/font files')
    if args.list_only:
        files = []
    # Do not let opening the serial device pulse reset before controls are set.
    port = serial.Serial(baudrate=args.baud, timeout=1, write_timeout=15)
    port.dtr = False
    port.rts = False
    port.port = args.port
    with port:
        reset_board(port)
        wait_line(port, ('Send PALSD', 'PALSD READY'), 30)
        port.write(b'\nPALSD\n') # Clear a possible partial ROM/reset noise byte.
        port.flush()
        wait_line(port, ('PALSD READY',))
        port.write(b'LIST\n')
        listing = []
        while True:
            line = wait_line(port, ('FILE ', 'LIST END'))
            if line == 'LIST END':
                break
            listing.append(line)
            print(line, flush=True)
        checks = []
        total = sum(path.stat().st_size for _, path in files)
        done = 0
        started = time.monotonic()
        for name, path in files:
            size, crc = path.stat().st_size, crc_file(path)
            port.write(f'STAT {name}\n'.encode())
            info = wait_line(port, ('STAT ',), 60).split()
            checks.append(dict(name=name, size=int(info[1]), crc32=int(info[2]),
                               expected_size=size, expected_crc32=crc,
                               matches=(int(info[1]), int(info[2])) == (size, crc)))
            if (int(info[1]), int(info[2])) == (size, crc):
                print(f'Verified, keeping {name} ({size} bytes)', flush=True)
                done += size
                continue
            if args.check:
                print(f'DIFFERENT OR MISSING: {name}: SD={info[1:]} expected={[size, crc]}', flush=True)
                continue
            print(f'Uploading {name} ({size} bytes)', flush=True)
            port.write(f'PUT {size} {crc} {name}\n'.encode())
            wait_line(port, ('SEND',))
            next_report = time.monotonic() + 15
            with path.open('rb') as source:
                while block := source.read(4096):
                    port.write(block)
                    wait_line(port, ('ACK',))
                    done += len(block)
                    if time.monotonic() >= next_report:
                        print(f'{done / total:.1%}, {done / 1048576:.1f}/{total / 1048576:.1f} MiB', flush=True)
                        next_report = time.monotonic() + 15
            wait_line(port, ('OK',))
            port.write(f'STAT {name}\n'.encode())
            info = wait_line(port, ('STAT ',), 60).split()
            if (int(info[1]), int(info[2])) != (size, crc):
                raise RuntimeError(f'SD readback checksum mismatch: {name}')
            checks[-1].update(size=size, crc32=crc, matches=True, updated=True)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(dict(listing=listing, checks=checks), indent=2), encoding='utf-8')
        print(f'Checked {len(files)} files in {time.monotonic() - started:.1f}s', flush=True)
        port.write(b'START\n')
        deadline = time.monotonic() + args.monitor
        while time.monotonic() < deadline:
            line = port.readline()
            if line:
                decoded = line.decode('utf-8', errors='replace').rstrip()
                if '\x00' not in decoded:
                    print(decoded, flush=True)


if __name__ == '__main__':
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')
    main()
