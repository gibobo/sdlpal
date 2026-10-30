"""Play a fixed route on the P4 through its UART test commands and log the output.

Needs a profiling build (tools/build-esp32.ps1 -Backend p4 -Profiling), which
accepts the commands of src/testcmd.h. The route resets the board, loads the
save slot with the most saves, walks, scrolls the item list, saves to test slot
6 (the game's menus show only slots 1-5, so no saved game is touched), fights
three battles (attacks, repeat, magic, an item), loads test slot 6 back and
checks that it restored the scene, position, cash and HP it was saved with.
Every UART line is written to the log with seconds since the start, for the
PALPROF summaries.
"""
import argparse
import queue
import re
import sys
import threading
import time
from pathlib import Path
import serial

# A save slot the game's menus never show; src/testcmd.c lets saves overwrite it.
TEST_SLOT = 6


class Board:
    """The P4 on a serial port: a reader thread logs every line and queues it."""

    def __init__(self, port_name, log_path):
        self.port = serial.Serial(baudrate=921600, timeout=0.2)
        self.port.dtr = False
        self.port.rts = False
        self.port.port = port_name
        self.port.open()
        self.log = open(log_path, 'w', encoding='utf-8', buffering=1)
        self.lines = queue.Queue()
        self.battle_done = None
        self.start = time.monotonic()
        self.running = True
        self.reader = threading.Thread(target=self.read_lines, daemon=True)
        self.reader.start()

    def read_lines(self):
        """Log and queue each complete line until closed."""
        while self.running:
            line = self.port.readline().decode('utf-8', errors='replace').strip()
            if line and '\x00' not in line:
                self.log.write('%8.3f %s\n' % (time.monotonic() - self.start, line))
                if line.startswith('PALCMD done battle'):
                    self.battle_done = line
                self.lines.put(line)

    def close(self):
        self.running = False
        self.reader.join(1)
        self.port.close()
        self.log.close()

    def reset(self):
        """Restart the board through the RTS line."""
        self.port.rts = True
        time.sleep(0.15)
        self.port.rts = False

    def note(self, text):
        """Mark a route step in the log and on the console."""
        self.log.write('%8.3f ROUTE %s\n' % (time.monotonic() - self.start, text))
        print('[%6.1f s] %s' % (time.monotonic() - self.start, text), flush=True)

    def wait_line(self, pattern, timeout):
        """Return the first queued line matching pattern; stop the route on timeout."""
        deadline = time.monotonic() + timeout
        regex = re.compile(pattern)
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=max(0.01, deadline - time.monotonic()))
            except queue.Empty:
                break
            if regex.search(line):
                return line
        raise RuntimeError('no line matching %r within %.0f s' % (pattern, timeout))

    def command(self, text, reply=None, timeout=15):
        """Send a test command and return its reply line; stop the route on an error.

        Replies echo their command, and lines queued before it are dropped, so
        replies to earlier unawaited taps are never taken for this one. The
        timeout allows for the UART output stalling for a few seconds, which
        was seen once while the game itself kept running.
        """
        while not self.lines.empty():
            self.lines.get_nowait()
        self.port.write((text + '\n').encode())
        echo = re.escape(text)
        line = self.wait_line(reply or r'^PALCMD ((ok|skip) %s|error .*: %s)$' % (echo, echo), timeout)
        if line.startswith('PALCMD error'):
            raise RuntimeError(line)
        return line

    def state(self):
        """The game state as a dict of the PALSTATE fields.

        The game answers only between frames, so allow for a long load first.
        """
        line = self.command('state', r'^PALSTATE ', timeout=30)
        return dict(field.split('=', 1) for field in line.split()[1:])

    def saves(self):
        """The saved times of slots 1 to 5; 0 is an empty slot."""
        line = self.command('saves', r'^PALSAVES ')
        return {int(k): int(v) for k, v in (field.split('=') for field in line.split()[1:])}

    def tap(self, key, count=1, gap=0.3):
        for _ in range(count):
            self.command('key press ' + key)
            time.sleep(gap)

    def hold(self, key, seconds):
        self.command('key hold ' + key)
        time.sleep(seconds)
        self.command('key release ' + key)

    def battle(self, team, field, opening=()):
        """Run a battle: send the opening keys, then attack until it ends.

        Battle keys use 'bkey', which the game drops once the battle is over,
        so a key sent as the battle ends cannot open a menu on the map.
        """
        self.battle_done = None
        self.command('battle %d %d' % (team, field))
        for key, wait in opening:
            time.sleep(wait)
            if self.battle_done is not None:
                return self.battle_done
            if key.startswith('hold '):
                self.command('bkey hold ' + key[5:])
                time.sleep(0.1)
                self.command('bkey release ' + key[5:])
            else:
                self.command('bkey press ' + key)
        deadline = time.monotonic() + 180
        while self.battle_done is None:
            if time.monotonic() > deadline:
                raise RuntimeError('battle %d did not end within 180 s' % team)
            self.port.write(b'bkey press search\n')
            time.sleep(0.4)
        return self.battle_done

    def finish_battle(self):
        """Attack until a battle the game started itself has ended."""
        deadline = time.monotonic() + 180
        while self.state()['battle'] != '0':
            if time.monotonic() > deadline:
                raise RuntimeError('a battle did not end within 180 s')
            self.command('bkey press search')
            time.sleep(0.4)

    def wait_walking(self):
        """Return the state once the game is back on the map with frames advancing.

        Stops the route when frames stand still, which means a menu or a
        dialog is waiting for a key.
        """
        deadline = time.monotonic() + 30
        while True:
            first = self.state()
            time.sleep(1)
            second = self.state()
            if second['battle'] == '0' and second['entering'] == '0' and int(second['frame']) > int(first['frame']):
                return second
            if time.monotonic() > deadline:
                raise RuntimeError('frames stopped advancing: %s' % second)


def route(board):
    """Play the fixed route from a reset."""
    board.note('reset')
    board.reset()
    board.wait_line(r'^PAL GAME READY', 60)
    # The trademark animation runs first; the splash screen then waits for a key.
    time.sleep(6)
    slots = board.saves()
    slot = max(slots, key=lambda s: slots[s])
    if slots[slot] == 0:
        raise RuntimeError('no saved game to load')
    board.note('load slot %d from the opening menu' % slot)
    board.tap('search', gap=3)
    board.tap('down')
    board.tap('search', gap=1)
    board.tap('down', slot - 1)
    board.tap('search', gap=0)
    board.wait_line(r'^PAL GAME RUNNING', 30)
    time.sleep(3)
    start = board.state()
    board.note('running: %s' % start)

    board.note('walk')
    for key in ('left', 'right', 'down', 'up'):
        board.hold(key, 0.6)
        time.sleep(0.5)
        if board.state()['battle'] != '0':
            board.note('walking met enemies')
            board.finish_battle()

    board.note('item list')
    board.command('item 61 3')
    board.command('item 72 1')
    board.tap('menu')
    board.tap('down', 2)
    board.tap('search', gap=0.5)
    board.tap('down')
    board.tap('search', gap=1)
    board.tap('down', 12, gap=0.25)
    board.tap('menu', 3, gap=0.5)
    saved = board.wait_walking()

    board.note('save to test slot %d: %s' % (TEST_SLOT, saved))
    board.command('save %d' % TEST_SLOT, timeout=10)
    time.sleep(1)

    board.note('battle 1: attacks')
    board.note(board.battle(1, 0))
    board.wait_walking()
    board.note('battle 2: repeat')
    board.note(board.battle(2, 0, opening=[('repeat', 1.5)]))
    board.wait_walking()
    board.note('battle 3: force, magic, item')
    board.note(board.battle(2, 0, opening=[('force', 1.5), ('hold left', 4), ('search', 0.5), ('search', 0.5),
                                           ('search', 0.5), ('useitem', 5), ('search', 0.5), ('search', 0.5)]))
    board.wait_walking()

    # Load the test save back: it must restore the state it was written from.
    board.note('load test slot %d' % TEST_SLOT)
    board.command('load %d' % TEST_SLOT)
    loaded = board.wait_walking()
    board.note('loaded: %s' % loaded)
    fields = ('scene', 'x', 'y', 'cash', 'hp')
    changed = [f for f in fields if loaded[f] != saved[f]]
    if changed:
        raise RuntimeError('test save did not restore %s' % ', '.join(changed))
    # Let the last PALPROF window arrive.
    time.sleep(2.5)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--port', default='COM3')
    parser.add_argument('--log', default='build/p4-drive.log')
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    Path(args.log).parent.mkdir(parents=True, exist_ok=True)
    board = Board(args.port, args.log)
    try:
        route(board)
        board.note('route finished')
    except RuntimeError as error:
        board.note('route stopped: %s' % error)
        sys.exit(1)
    finally:
        board.close()


if __name__ == '__main__':
    main()
