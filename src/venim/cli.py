import argparse
import glob
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from venim import core, db
from venim.vnb import parse_file

VERSION = '0.1.0'
RECIPE_DIRS = ['packages', 'examples']


def find_recipe(name, repo='.'):
    for base in RECIPE_DIRS:
        for path in glob.glob(os.path.join(repo, base, '**', name + '.vnb'), recursive=True):
            return path
    return None


def cmd_install(args):
    rec_path = find_recipe(args.package, args.repo)
    if not rec_path and os.path.exists(args.package):
        rec_path = args.package
    if not rec_path:
        print(f'package not found: {args.package}', file=sys.stderr)
        return 1
    rec = parse_file(rec_path)
    use_binary = args.binary or (not rec.get('source') and rec.get('binary'))
    if args.source:
        use_binary = False
    for dep in rec.get('depends', []):
        if not db.get_pkg(args.root, dep):
            print(f'dependency missing: {dep} (install it first)')
    if use_binary:
        pkgdir = core.install_binary(args.root, rec, verbose=args.verbose)
    else:
        dest = core.build_source(args.root, rec, verbose=args.verbose)
        pkgdir = core.install_destdir(args.root, rec, dest)
    if not args.quiet:
        print(f'installed {rec["name"]} {rec.get("version", "")} -> {pkgdir}')
    return 0


def cmd_remove(args):
    rev = db.reverse_deps(args.root, args.package)
    if rev and not args.force:
        print(f'refused: needed by {", ".join(rev)} (use --force)', file=sys.stderr)
        return 1
    pkg = db.get_pkg(args.root, args.package)
    if not pkg:
        print(f'not installed: {args.package}', file=sys.stderr)
        return 1
    core.unlink_package(args.root, pkg['name'], pkg['version'])
    db.record_remove(args.root, pkg['name'], pkg['version'])
    if not args.quiet:
        print(f'removed {pkg["name"]} {pkg["version"]}')
    return 0


def cmd_list(args):
    rows = db.list_pkgs(args.root)
    if args.json:
        print(json.dumps([{'name': n, 'version': v, 'active': bool(a), 'desc': d}
                          for n, v, a, d in rows], indent=2))
    else:
        for n, v, a, d in rows:
            print(f'{n} {v}{"" if a else " (inactive)"} - {d}')
    return 0


def cmd_info(args):
    rec_path = find_recipe(args.package, args.repo)
    inst = db.get_pkg(args.root, args.package)
    if args.json:
        print(json.dumps({'recipe': rec_path, 'installed': inst}, indent=2))
        return 0
    if rec_path:
        rec = parse_file(rec_path)
        print(f'{rec["name"]} {rec.get("version", "")}')
        print(f'  desc: {rec.get("description", "")}')
        print(f'  recipe: {rec_path}')
        print(f'  depends: {" ".join(rec.get("depends", [])) or "-"}')
    if inst:
        print(f'  installed: {inst["version"]} ({len(inst["files"])} files)')
    elif not rec_path:
        print('unknown package')
        return 1
    return 0


def cmd_search(args):
    hits = []
    for base in RECIPE_DIRS:
        for path in glob.glob(os.path.join(args.repo, base, '**', '*.vnb'), recursive=True):
            name = os.path.splitext(os.path.basename(path))[0]
            if args.query.lower() in name.lower():
                hits.append((name, path))
    if args.json:
        print(json.dumps(hits))
    else:
        for n, p in sorted(hits):
            print(f'{n} - {p}')
    return 0


def cmd_depends(args):
    rec_path = find_recipe(args.package, args.repo)
    if not rec_path:
        return 1
    print(' '.join(parse_file(rec_path).get('depends', [])))
    return 0


def cmd_provides(args):
    rows = db.list_pkgs(args.root)
    for n, v, _, _ in rows:
        pkg = db.get_pkg(args.root, n, v)
        for f in pkg['files']:
            if f.endswith(args.file) or args.file in f:
                print(f'{n} {v}: {f}')
    return 0


def cmd_verify(args):
    pkg = db.get_pkg(args.root, args.package)
    if not pkg:
        print('not installed', file=sys.stderr)
        return 1
    missing = []
    pkgdir = os.path.join(args.root, f'venim/packages/{pkg["name"]}/{pkg["version"]}')
    for f in pkg['files']:
        if f.startswith('link:'):
            p = os.path.join(args.root, f[5:])
            if not os.path.islink(p):
                missing.append(f)
        else:
            if not os.path.exists(os.path.join(pkgdir, f)):
                missing.append(f)
    if missing:
        print('BROKEN:')
        for m in missing:
            print(f'  {m}')
        return 1
    print('ok')
    return 0


def cmd_doctor(args):
    bad = 0
    for n, v, a, _ in db.list_pkgs(args.root):
        if not a:
            continue
        pkg = db.get_pkg(args.root, n, v)
        for dep in pkg['depends']:
            if not db.get_pkg(args.root, dep):
                print(f'{n}: missing dep {dep}')
                bad += 1
    usrb = os.path.join(args.root, 'usr/bin')
    if os.path.isdir(usrb):
        for e in os.listdir(usrb):
            p = os.path.join(usrb, e)
            if os.path.islink(p) and not os.path.exists(p):
                print(f'dangling link: usr/bin/{e}')
                bad += 1
    print('ok' if bad == 0 else f'{bad} problem(s)')
    return 1 if bad else 0


def cmd_repair(args):
    target = args.root_arg or args.root
    count = 0
    for n, v, a, _ in db.list_pkgs(target):
        if not a:
            continue
        core.link_package(target, n, v)
        count += 1
    print(f'relinked {count} package(s) under {target}')
    return 0


def cmd_build(args):
    rec = parse_file(args.recipe)
    dest = core.build_source(args.root, rec, verbose=args.verbose)
    print(dest)
    return 0


def cmd_clean(args):
    for d in ('venim/build', 'venim/cache'):
        p = os.path.join(args.root, d)
        if os.path.isdir(p):
            shutil.rmtree(p)
            os.makedirs(p)
    print('cleaned')
    return 0


def cmd_update(args):
    print('recipe index is local (packages/ + examples/); nothing to fetch yet')
    return 0


def cmd_upgrade(args):
    print('upgrade: re-install listed packages from recipes')
    code = 0
    for n, v, a, _ in db.list_pkgs(args.root):
        if not a:
            continue
        rec_path = find_recipe(n, args.repo)
        if not rec_path:
            continue
        rec = parse_file(rec_path)
        if rec.get('version') != v:
            print(f'{n}: {v} -> {rec.get("version")}')
            dest = core.build_source(args.root, rec, verbose=args.verbose)
            core.install_destdir(args.root, rec, dest)
    return code


def main(argv=None):
    ap = argparse.ArgumentParser(prog='venim')
    ap.add_argument('--root', default=os.environ.get('VENIM_ROOT', '/'))
    ap.add_argument('--repo', default='.')
    ap.add_argument('--source', action='store_true')
    ap.add_argument('--binary', action='store_true')
    ap.add_argument('--verbose', action='store_true')
    ap.add_argument('--quiet', action='store_true')
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--version', action='store_true')
    ap.add_argument('--root-arg', dest='root_arg', default=None)
    ap.add_argument('command', nargs='?')
    ap.add_argument('operand', nargs='*')
    ns = ap.parse_args(argv)
    if ns.version or ns.command in (None, '--version'):
        if ns.command is None and not ns.version:
            ap.print_help()
            return 0
        print(f'venim {VERSION}')
        return 0
    args = ns
    table = {
        'install': (cmd_install, 'package'), 'remove': (cmd_remove, 'package'),
        'list': (cmd_list, None), 'info': (cmd_info, 'package'),
        'search': (cmd_search, 'query'), 'depends': (cmd_depends, 'package'),
        'provides': (cmd_provides, 'file'), 'verify': (cmd_verify, 'package'),
        'clean': (cmd_clean, None), 'build': (cmd_build, 'recipe'),
        'doctor': (cmd_doctor, None), 'repair': (cmd_repair, None),
        'update': (cmd_update, None), 'upgrade': (cmd_upgrade, None),
    }
    if ns.command not in table:
        print(f'unknown command: {ns.command}', file=sys.stderr)
        return 1
    fn, field = table[ns.command]
    if field and ns.operand:
        setattr(args, field, ns.operand[0])
    elif field:
        print(f'{ns.command} needs an argument', file=sys.stderr)
        return 1
    return fn(args)


if __name__ == '__main__':
    raise SystemExit(main())
