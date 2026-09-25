import os
import platform
import re
import subprocess


BAD_LINK_TARGETS = ('/tmp/', '/venim/build/', '/venim/cache/')


def _run(args):
    try:
        return subprocess.run(args, capture_output=True, text=True, timeout=20)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return None


def adapt_tree(staging):
    fixed = []
    for dirpath, dirnames, filenames in os.walk(staging):
        for fn in filenames:
            full = os.path.join(dirpath, fn)
            if os.path.islink(full):
                continue
            rel = os.path.relpath(full, staging)
            if _fix_script(full):
                fixed.append(rel + ' (shebang)')
            if fn.endswith('.desktop'):
                if _fix_desktop(full):
                    fixed.append(rel + ' (desktop)')
        for d in list(dirnames):
            full = os.path.join(dirpath, d)
            if os.path.islink(full):
                target = os.readlink(full)
                if target.startswith(BAD_LINK_TARGETS) or '/build/' in target:
                    raise ValueError(f'unsafe symlink {full} -> {target}')
    return fixed


def _fix_script(path):
    try:
        with open(path, 'rb') as f:
            head = f.read(512)
    except OSError:
        return False
    if not head.startswith(b'#!'):
        return False
    try:
        line, rest = head.split(b'\n', 1)
    except ValueError:
        return False
    try:
        text = line.decode()
    except UnicodeDecodeError:
        return False
    if text.strip() in ('#!/usr/bin/python3', '#!/usr/bin/python'):
        new = '#!/usr/bin/env python3\n'
    elif text.strip() == '#!/bin/bash' and not os.path.exists('/bin/bash'):
        new = '#!/usr/bin/env bash\n'
    else:
        return False
    with open(path, 'rb') as f:
        data = f.read()
    nl = data.find(b'\n')
    with open(path, 'wb') as f:
        f.write(new.encode() + data[nl + 1:])
    return True


def _fix_desktop(path):
    try:
        with open(path) as f:
            data = f.read()
    except OSError:
        return False
    new = re.sub(r'^Exec=/usr/bin/(\S+)', r'Exec=\1', data, flags=re.M)
    new = re.sub(r'^Exec=/usr/sbin/(\S+)', r'Exec=\1', new, flags=re.M)
    if new != data:
        with open(path, 'w') as f:
            f.write(new)
        return True
    return False


def fix_elf_rpath(path, pkgdir):
    r = _run(['patchelf', '--print-rpath', path])
    if r is None or r.returncode != 0:
        return False
    old = r.stdout.strip()
    if '/tmp' in old or '/build' in old or '/home/' in old:
        libdir = os.path.join(pkgdir, 'lib')
        new = libdir if os.path.isdir(libdir) else '$ORIGIN/../lib:$ORIGIN'
        w = _run(['patchelf', '--set-rpath', new, path])
        return w is not None and w.returncode == 0
    return False


def validate_tree(staging, pkgdir):
    errors = []
    machine = platform.machine()
    for dirpath, _, filenames in os.walk(staging):
        for fn in filenames:
            full = os.path.join(dirpath, fn)
            if os.path.islink(full):
                target = os.readlink(full)
                if target.startswith(BAD_LINK_TARGETS):
                    errors.append(f'{full}: unsafe symlink -> {target}')
                continue
            if _is_elf(full):
                r = _run(['readelf', '-h', full])
                if r is None or r.returncode != 0:
                    errors.append(f'{full}: unreadable ELF')
                    continue
                if machine == 'x86_64' and 'X86-64' not in r.stdout:
                    errors.append(f'{full}: wrong arch (need x86_64)')
                d = _run(['readelf', '-d', full])
                if d and ('NEEDED' not in d.stdout and os.access(full, os.X_OK)):
                    pass
                fix_elf_rpath(full, pkgdir)
            if os.access(full, os.X_OK) is False and full.endswith('.so'):
                pass
    return errors


def _is_elf(path):
    try:
        with open(path, 'rb') as f:
            return f.read(4) == b'\x7fELF'
    except OSError:
        return False


def unpack(archive, dest):
    os.makedirs(dest, exist_ok=True)
    if archive.endswith('.zip'):
        r = subprocess.run(['unzip', '-q', archive, '-d', dest])
    elif archive.endswith('.zst') or archive.endswith('.zstd'):
        r = subprocess.run(f'tar --use-compress-program=unzstd -xf {archive} -C {dest}',
                           shell=True)
    else:
        r = subprocess.run(['tar', '-xf', archive, '-C', dest])
    if r.returncode != 0:
        raise RuntimeError(f'unpack failed: {archive}')
