import os
import sys
import tempfile
from pathlib import Path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))


def test_parser_minimal(tmp_path=None):
    from venim.vnb import parse_file
    tmp_path = Path(tmp_path) if tmp_path else Path(tempfile.mkdtemp())
    p = tmp_path / 'a.vnb'
    p.write_text('package "nano" {\n version = "8.6"\n depends { "glibc" "ncurses" }\n'
                 ' build { command "make" }\n}\n')
    rec = parse_file(str(p))
    assert rec['name'] == 'nano'
    assert rec['version'] == '8.6'
    assert 'glibc' in rec['depends']
    assert rec['build'] == ['make']


def test_install_hello(tmp_path=None):
    from venim import core, db
    from venim.vnb import parse_file
    root = str(tmp_path) if tmp_path else tempfile.mkdtemp()
    rec = {'name': 'hello-venim', 'version': '0.1.0', 'description': 't'}
    dest = core.build_source(root, rec)
    pkgdir = core.install_destdir(root, rec, dest)
    assert os.path.exists(os.path.join(pkgdir, 'bin/hello'))
    link = os.path.join(root, 'usr/bin/hello')
    assert os.path.islink(link)
    assert db.get_pkg(root, 'hello-venim') is not None
    core.unlink_package(root, 'hello-venim', '0.1.0')
    assert not os.path.exists(pkgdir)


if __name__ == '__main__':
    test_parser_minimal()
    print('ok: parser')
    test_install_hello()
    print('ok: install-hello')
