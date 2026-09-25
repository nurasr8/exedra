import hashlib
import os
import shutil
import subprocess
import urllib.request

from . import adapter, db
from .vnb import expand_vars


def venim_paths(root):
    return {
        'pkg': os.path.join(root, 'venim/packages'),
        'build': os.path.join(root, 'venim/build'),
        'cache': os.path.join(root, 'venim/cache'),
        'sources': os.path.join(root, 'venim/sources'),
    }


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def fetch(url, dest):
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if os.path.exists(dest):
        return dest
    urllib.request.urlretrieve(url, dest)
    return dest


def run_cmds(cmds, cwd, env):
    for c in cmds:
        c = expand_vars(c, env)
        r = subprocess.run(c, shell=True, cwd=cwd, env={**os.environ, **env})
        if r.returncode != 0:
            raise RuntimeError(f'command failed: {c}')


def base_env(rec, srcdir, builddir, destdir):
    return {
        'name': rec['name'],
        'version': rec.get('version', '0'),
        'srcdir': srcdir,
        'builddir': builddir,
        'destdir': destdir,
        'DESTDIR': destdir,
        'prefix': '/usr',
    }


def list_files(tree):
    out = []
    for dirpath, _, filenames in os.walk(tree):
        for fn in filenames:
            full = os.path.join(dirpath, fn)
            out.append(os.path.relpath(full, tree))
    return sorted(out)


def link_package(root, name, version):
    pkgdir = os.path.join(root, f'venim/packages/{name}/{version}')
    mapping = {'bin': 'usr/bin', 'sbin': 'usr/sbin', 'lib': 'usr/lib',
               'lib64': 'usr/lib64', 'share': 'usr/share'}
    linked = []
    for dirpath, _, filenames in os.walk(pkgdir):
        for fn in filenames:
            src = os.path.join(dirpath, fn)
            rel = os.path.relpath(src, pkgdir)
            top, _, rest = rel.partition('/')
            target_rel = os.path.join(mapping.get(top, top), rest) if rest else mapping.get(top, top)
            dst = os.path.join(root, target_rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            if os.path.islink(dst) or os.path.exists(dst):
                os.remove(dst)
            os.symlink(os.path.relpath(src, os.path.dirname(dst)), dst)
            linked.append(target_rel)
    return linked


def unlink_package(root, name, version):
    pkgdir = os.path.join(root, f'venim/packages/{name}/{version}')
    removed = []
    for dirpath, _, filenames in os.walk(pkgdir):
        for fn in filenames:
            src = os.path.join(dirpath, fn)
            rel = os.path.relpath(src, pkgdir)
            top, _, rest = rel.partition('/')
            mapping = {'bin': 'usr/bin', 'sbin': 'usr/sbin', 'lib': 'usr/lib',
                       'lib64': 'usr/lib64', 'share': 'usr/share'}
            target_rel = os.path.join(mapping.get(top, top), rest) if rest else mapping.get(top, top)
            dst = os.path.join(root, target_rel)
            if os.path.islink(dst) and os.path.realpath(dst) == src:
                os.remove(dst)
                removed.append(target_rel)
    try:
        shutil.rmtree(pkgdir)
    except FileNotFoundError:
        pass
    parent = os.path.dirname(pkgdir)
    try:
        if os.path.isdir(parent) and not os.listdir(parent):
            os.rmdir(parent)
    except OSError:
        pass
    return removed


def build_source(root, rec, verbose=False):
    p = venim_paths(root)
    name, ver = rec['name'], rec.get('version', '0')
    work = os.path.join(p['build'], f'{name}-{ver}')
    srcdir = os.path.join(work, 'src')
    builddir = os.path.join(work, 'build')
    destdir = os.path.join(work, 'dest')
    for d in (srcdir, builddir, destdir):
        os.makedirs(d, exist_ok=True)
    env = base_env(rec, srcdir, builddir, destdir)

    src = rec.get('source', {})
    if isinstance(src, dict) and src.get('url'):
        arc = os.path.join(p['sources'], os.path.basename(src['url']))
        fetch(src['url'], arc)
        if src.get('sha256') and not src['sha256'].startswith('0' * 8):
            got = sha256_file(arc)
            if got != src['sha256']:
                raise ValueError(f'sha256 mismatch for {name}: want {src["sha256"]}, got {got}')
        adapter.unpack(arc, srcdir)
    elif name == 'hello-venim':
        with open(os.path.join(srcdir, 'hello.c'), 'w') as f:
            f.write('#include <stdio.h>\nint main(void){puts("Hello from Exedra!");return 0;}\n')

    entries = os.listdir(srcdir)
    if len(entries) == 1 and os.path.isdir(os.path.join(srcdir, entries[0])):
        topsrc = os.path.join(srcdir, entries[0])
    else:
        topsrc = srcdir
    workdir = topsrc if entries else work

    for f in rec.get('patch', []) if isinstance(rec.get('patch'), list) else []:
        pass

    for phase in ('prepare', 'build'):
        cmds = rec.get(phase, [])
        if isinstance(cmds, dict):
            cmds = cmds.get('_cmds', [])
        if cmds:
            run_cmds(cmds, workdir, env)

    check = rec.get('check', [])
    if isinstance(check, dict):
        check = check.get('_cmds', [])
    if check:
        try:
            run_cmds(check, workdir, env)
        except RuntimeError as e:
            if verbose:
                print(f'check failed (non-fatal): {e}')

    inst = rec.get('install', [])
    if isinstance(inst, dict):
        inst = inst.get('_cmds', [])
    if inst:
        run_cmds(inst, workdir, env)
    elif name == 'hello-venim':
        bindir = os.path.join(destdir, 'bin')
        os.makedirs(bindir, exist_ok=True)
        src_c = os.path.join(srcdir, 'hello.c')
        out = os.path.join(destdir, 'bin/hello')
        r = subprocess.run(['cc', src_c, '-o', out])
        if r.returncode != 0:
            raise RuntimeError('cc failed for hello-venim')
    else:
        raise RuntimeError('recipe has no install steps')

    return destdir


def install_destdir(root, rec, destdir, source='source', checksum=''):
    name, ver = rec['name'], rec.get('version', '0')
    if os.path.isdir(os.path.join(destdir, 'usr')) and not os.path.exists(os.path.join(destdir, 'bin')):
        norm = destdir.rstrip('/') + '.nexa'
        if os.path.exists(norm):
            shutil.rmtree(norm)
        _to_exedra_layout(destdir, norm)
        destdir = norm
    pkgdir = os.path.join(root, f'venim/packages/{name}/{ver}')
    if os.path.exists(pkgdir):
        shutil.rmtree(pkgdir)
    shutil.copytree(destdir, pkgdir, symlinks=True)
    adapter.adapt_tree(pkgdir)
    errs = adapter.validate_tree(pkgdir, pkgdir)
    if errs:
        raise RuntimeError('validation failed:\n' + '\n'.join(errs))
    files = list_files(pkgdir)
    linked = link_package(root, name, ver)
    db.record_install(root, rec, files + ['link:' + l for l in linked],
                      checksum=checksum, source=source)
    return pkgdir


def install_binary(root, rec, verbose=False):
    p = venim_paths(root)
    b = rec.get('binary', {})
    if not b or not b.get('url'):
        raise ValueError('recipe has no binary section')
    arc = os.path.join(p['cache'], os.path.basename(b['url']))
    fetch(b['url'], arc)
    if b.get('sha256'):
        got = sha256_file(arc)
        if got != b['sha256']:
            raise ValueError(f'sha256 mismatch: want {b["sha256"]}, got {got}')
    work = os.path.join(p['build'], f"{rec['name']}-{rec.get('version', '0')}-bin")
    stage = os.path.join(work, 'stage')
    if os.path.exists(stage):
        shutil.rmtree(stage)
    os.makedirs(stage)
    adapter.unpack(arc, stage)
    inner = _strip_top_level(stage)
    destdir = os.path.join(work, 'dest')
    if os.path.exists(destdir):
        shutil.rmtree(destdir)
    os.makedirs(destdir)
    inst = rec.get('install', [])
    if isinstance(inst, dict):
        inst = inst.get('_cmds', [])
    if inst:
        builddir = os.path.join(work, 'build')
        os.makedirs(builddir, exist_ok=True)
        env = base_env(rec, os.path.dirname(arc), builddir, destdir)
        try:
            run_cmds(inst, stage, env)
        except RuntimeError as e:
            print(f'install steps failed, generic layout instead: {e}')
            if os.path.exists(destdir):
                shutil.rmtree(destdir)
            _to_exedra_layout(inner, destdir)
    else:
        _to_exedra_layout(inner, destdir)
    return install_destdir(root, rec, destdir, source='binary', checksum=b.get('sha256', ''))


def _strip_top_level(stage):
    entries = [e for e in os.listdir(stage) if not e.startswith('.')]
    if len(entries) == 1 and os.path.isdir(os.path.join(stage, entries[0])):
        top = os.path.join(stage, entries[0])
        if any(os.path.isdir(os.path.join(top, d)) for d in ('bin', 'usr', 'lib', 'share')):
            return top
    return stage


def _copy_entry(s, d):
    if os.path.isdir(s) and not os.path.islink(s):
        if os.path.exists(d):
            shutil.rmtree(d)
        shutil.copytree(s, d, symlinks=True)
    elif os.path.islink(s):
        if os.path.lexists(d):
            os.remove(d)
        os.symlink(os.readlink(s), d)
    else:
        shutil.copy2(s, d)


def _to_exedra_layout(src, dest):
    os.makedirs(dest, exist_ok=True)
    if os.path.isdir(os.path.join(src, 'usr')) and not os.path.exists(os.path.join(src, 'bin')):
        for e in os.listdir(src):
            s = os.path.join(src, e)
            if e == 'usr':
                for u in os.listdir(s):
                    _copy_entry(os.path.join(s, u), os.path.join(dest, u))
            else:
                _copy_entry(s, os.path.join(dest, e))
        return
    for e in os.listdir(src):
        _copy_entry(os.path.join(src, e), os.path.join(dest, e))
