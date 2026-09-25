import json
import os
import sqlite3


SCHEMA = """
CREATE TABLE IF NOT EXISTS packages(
  name TEXT,
  version TEXT,
  description TEXT DEFAULT '',
  active INTEGER DEFAULT 1,
  files TEXT DEFAULT '[]',
  depends TEXT DEFAULT '[]',
  checksum TEXT DEFAULT '',
  source TEXT DEFAULT '',
  PRIMARY KEY(name, version)
);
"""


def open_db(root):
    path = os.path.join(root, 'venim/db/packages.db')
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
    except (PermissionError, OSError):
        con = sqlite3.connect(':memory:')
        con.execute(SCHEMA)
        return con
    try:
        con = sqlite3.connect(path)
    except (sqlite3.OperationalError, PermissionError, OSError):
        con = sqlite3.connect(':memory:')
        con.execute(SCHEMA)
        return con
    con.execute(SCHEMA)
    return con


def record_install(root, rec, files, checksum='', source=''):
    con = open_db(root)
    con.execute('UPDATE packages SET active=0 WHERE name=?', (rec['name'],))
    con.execute(
        'INSERT OR REPLACE INTO packages(name,version,description,active,files,depends,checksum,source)'
        ' VALUES(?,?,?,?,?,?,?,?)',
        (rec['name'], rec.get('version', '0'),
         rec.get('description', ''), 1,
         json.dumps(files),
         json.dumps(rec.get('depends', [])),
         checksum, source))
    con.commit()
    con.close()


def record_remove(root, name, version=None):
    con = open_db(root)
    if version:
        con.execute('DELETE FROM packages WHERE name=? AND version=?', (name, version))
    else:
        con.execute('DELETE FROM packages WHERE name=?', (name,))
    con.commit()
    con.close()


def get_pkg(root, name, version=None):
    con = open_db(root)
    if version:
        row = con.execute('SELECT name,version,description,active,files,depends FROM packages'
                          ' WHERE name=? AND version=?', (name, version)).fetchone()
    else:
        row = con.execute('SELECT name,version,description,active,files,depends FROM packages'
                          ' WHERE name=? AND active=1', (name,)).fetchone()
    con.close()
    if not row:
        return None
    return {'name': row[0], 'version': row[1], 'description': row[2],
            'active': row[3], 'files': json.loads(row[4]), 'depends': json.loads(row[5])}


def list_pkgs(root):
    con = open_db(root)
    rows = con.execute('SELECT name,version,active,description FROM packages ORDER BY name').fetchall()
    con.close()
    return rows


def reverse_deps(root, name):
    con = open_db(root)
    rows = con.execute('SELECT name,depends FROM packages WHERE active=1').fetchall()
    con.close()
    out = []
    for pkg, deps in rows:
        if name in json.loads(deps or '[]'):
            out.append(pkg)
    return out
