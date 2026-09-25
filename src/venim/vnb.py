import re
import shlex


def tokenize(text):
    pat = re.compile(r'\s*(?:"([^"]*)"|([A-Za-z_][A-Za-z0-9_\-\.]*)|(\$\{[^}]*\})|([{}=]))')
    pos = 0
    toks = []
    n = len(text)
    while pos < n:
        if text[pos] == '#':
            while pos < n and text[pos] != '\n':
                pos += 1
            continue
        if text[pos].isspace():
            pos += 1
            continue
        m = pat.match(text, pos)
        if not m:
            raise ValueError(f'parse error at: {text[pos:pos+40]!r}')
        s, ident, var, sym = m.groups()
        pos = m.end()
        if s is not None:
            toks.append(('STR', s))
        elif ident is not None:
            toks.append(('ID', ident))
        elif var is not None:
            toks.append(('ID', var))
        elif sym is not None:
            toks.append((sym, sym))
    return toks


class Parser:
    def __init__(self, toks):
        self.toks = toks
        self.pos = 0

    def peek(self):
        return self.toks[self.pos] if self.pos < len(self.toks) else (None, None)

    def next(self):
        t = self.peek()
        self.pos += 1
        return t

    def expect(self, kind):
        k, v = self.next()
        if k != kind:
            raise ValueError(f'expected {kind}, got {k}:{v}')
        return v

    def parse_block(self):
        out = {}
        items = []
        cmds = []
        while True:
            k, v = self.peek()
            if k == '}':
                self.next()
                break
            if k is None:
                raise ValueError('unexpected eof in block')
            if k == 'ID' and v == 'command':
                self.next()
                ck, cv = self.next()
                if ck not in ('STR', 'ID'):
                    raise ValueError('command expects string')
                cmds.append(cv)
                continue
            if k in ('STR', 'ID'):
                nxt = self.toks[self.pos + 1] if self.pos + 1 < len(self.toks) else (None, None)
                if nxt[0] in ('=', '{'):
                    key = self.next()[1]
                    nk, nv = self.peek()
                    if nk == '{':
                        self.next()
                        out[key] = self.parse_block()
                    elif nk == '=':
                        self.next()
                        vk, vv = self.next()
                        if vk not in ('STR', 'ID'):
                            raise ValueError(f'value expected after {key} =')
                        out[key] = vv
                    continue
                items.append(self.next()[1])
                continue
            raise ValueError(f'unexpected token {k}:{v}')
        if items and not out and not cmds:
            return items
        if items:
            out['_list'] = items
            if len(out) == 1:
                return items
        if cmds:
            out['_cmds'] = cmds
        return out


def parse_file(path):
    with open(path) as f:
        text = f.read()
    toks = tokenize(text)
    p = Parser(toks)
    k, v = p.next()
    if k != 'ID' or v != 'package':
        raise ValueError('recipe must start with package "name"')
    nk, nv = p.next()
    if nk != 'STR':
        raise ValueError('package name must be a string')
    name = nv
    p.expect('{')
    body = p.parse_block()
    rec = {'name': name}
    rec.update(body)
    for b in ('build', 'install', 'prepare', 'check'):
        if isinstance(rec.get(b), dict) and '_cmds' in rec[b]:
            rec[b] = rec[b]['_cmds']
    for b in ('depends', 'makedepends', 'provides', 'conflicts'):
        v = rec.get(b)
        if isinstance(v, dict):
            vals = []
            for x in v.values():
                if isinstance(x, list):
                    vals += x
                elif isinstance(x, str):
                    vals.append(x)
            rec[b] = vals
    return rec


def expand_vars(cmd, env):
    def sub(m):
        key = m.group(1) or m.group(2)
        return shlex.quote(env.get(key, m.group(0)))
    return re.sub(r'\$\{([^}]+)\}|\$([A-Za-z_][A-Za-z0-9_]*)', sub, cmd)
