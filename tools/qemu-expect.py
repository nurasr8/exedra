#!/usr/bin/env python3
# Drive qemu's serial console: wait for EXPECT bytes, send SEND, repeat.
# Script file format: blocks separated by a line "---", each block:
#   line1: EXPECT substring (utf-8, empty = send immediately)
#   rest:  bytes to send (\\n escapes supported)
# Usage: qemu-expect.py <timeout_s> <script> <log> -- <qemu...>
import os
import select
import subprocess
import sys
import time


def parse(path):
    steps = []
    for block in open(path, encoding='utf-8').read().split('\n---\n'):
        lines = block.split('\n')
        exp = lines[0].encode()
        if len(lines) > 1:
            send = '\n'.join(lines[1:]).replace('\\n', '\n')
            if not send.endswith('\n'):
                send += '\n'
        else:
            send = ''
        steps.append((exp, send.encode()))
    return [s for s in steps if s[0] or s[1]]


def main():
    timeout = int(sys.argv[1])
    steps = parse(sys.argv[2])
    log = open(sys.argv[3], 'wb')
    cmd = sys.argv[sys.argv.index('--') + 1:]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    out = b''
    end = time.time() + timeout
    ok = True
    for exp, send in steps:
        found = not exp
        while not found and time.time() < end:
            r, _, _ = select.select([p.stdout], [], [], 1.0)
            if r:
                piece = os.read(p.stdout.fileno(), 65536)
                if not piece:
                    break
                log.write(piece)
                log.flush()
                out += piece[-200000:]
                if exp in out:
                    found = True
            if p.poll() is not None and not found:
                break
        if not found:
            ok = False
            break
        if send:
            time.sleep(0.5)
            try:
                p.stdin.write(send)
                p.stdin.flush()
            except BrokenPipeError:
                ok = False
                break
            out = b''
    # drain remaining output briefly
    stop = time.time() + 8
    while time.time() < stop:
        r, _, _ = select.select([p.stdout], [], [], 1.0)
        if not r:
            break
        piece = os.read(p.stdout.fileno(), 65536)
        if not piece:
            break
        log.write(piece)
        log.flush()
    log.close()
    try:
        p.terminate()
    except Exception:
        pass
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
