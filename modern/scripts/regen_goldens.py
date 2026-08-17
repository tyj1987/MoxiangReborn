"""Regenerate InterfaceScript goldens from legacy .bin files."""
import os, struct, json, sys

src_dir = r'C:/moxiang/墨香【源码配套资源】/PlayDH/Image/InterfaceScript'
out_dir = r'C:/moxiang/modern/tests/fixtures/interface_script'

def decrypt(data, t):
    out = bytearray(data)
    for i in range(len(out)):
        v = (out[i] - (i & 0xff)) & 0xff
        if t != 0 and (i % t) == 0:
            v = (v - (t & 0xff)) & 0xff
        out[i] = v
    return bytes(out)

class Parser:
    def __init__(self, payload):
        self.p = payload
        self.pos = 0
        self.L = len(payload)

    def is_ws(self, c): return c in (' ', '\t', '\n', '\r')
    def is_word(self, c): return c.isalnum() or c in ('$', '#', '_', '.')
    def is_brace(self, c): return c in ('{', '}', '(', ')')

    def next_tok(self):
        while self.pos < self.L:
            c = self.p[self.pos]
            if c == '@':
                while self.pos < self.L and self.p[self.pos] != '\n':
                    self.pos += 1
                continue
            if self.is_ws(c):
                self.pos += 1; continue
            if self.is_brace(c):
                self.pos += 1; return c
            if self.is_word(c):
                start = self.pos
                while self.pos < self.L and self.is_word(self.p[self.pos]):
                    self.pos += 1
                return self.p[start:self.pos]
            self.pos += 1
        return None

    def parse_int(self):
        t = self.next_tok()
        if t is None: return None
        # Mirror legacy atoi(): silently 0 for non-numeric tokens like "O".
        try:
            return int(t)
        except ValueError:
            try:
                # strtol-like: parse leading digits, ignore rest
                import re
                m = re.match(r'^[+-]?\d+', t)
                if m:
                    return int(m.group(0))
                return 0
            except Exception:
                return 0

    def parse_4ints(self):
        v = [0,0,0,0]
        for i in range(4):
            x = self.parse_int()
            if x is None: return None
            v[i] = x
        return v

    def parse_image_idx(self):
        saved = self.pos
        tok = self.next_tok()
        if tok == '(':
            v5 = [0,0,0,0,0]; ok = True
            for i in range(5):
                x = self.parse_int()
                if x is None: ok = False; break
                v5[i] = x
            close = self.next_tok()
            if ok and close == ')':
                return v5[0], v5[1:], self.pos
            self.pos = saved
        else:
            self.pos = saved
        idx = self.parse_int()
        if idx is None: return None, None, self.pos
        return idx, None, self.pos

    def skip_eol(self):
        while self.pos < self.L and self.p[self.pos] != '\n':
            self.pos += 1

    INT_PROPS = {
        'FGCOLOR':3,'TEXTCOLOR':6,'SHADOWCOLOR':3,
        'EDITSIZE':2,'SPINSIZE':2,'TEXTRECT':4,
        'TEXTSHADOWRECT':4,'TEXTXY':2,'CAPTIONRECT':4,
        'LIMITBYTES':1,'TEXTALIGN':1,'MAXLINE':1,
        'LISTMAXLINE':1,'LINEHEIGHT':1,'BTNTEXTANI':2,
        'COORD':2,'ITEMTOOLTIP':1,'TOOLTIPCOL':3,'SHADOWTEXT':2,
    }
    BOOL_PROPS = {'SHADOW','AUTOSCROLL','PASSIVE','READONLY','SECRET'}

    def apply_prop(self, node, prop):
        if prop in ('POINT','POINT_'):
            v = self.parse_4ints()
            if v is None: return False
            if prop == 'POINT': node['point'] = v
            else: node['point_low'] = v
            return True
        if prop == 'CAPTIONRECT':
            v = self.parse_4ints()
            if v is None: return False
            node['caption_rect'] = v; return True
        if prop == 'BASICIMAGE':
            idx, rect, newpos = self.parse_image_idx()
            if idx is None: return False
            self.pos = newpos
            node['basic_image_idx'] = idx
            if rect is not None: node['basic_image_rect'] = rect
            return True
        if prop in ('OVERIMAGE','PRESSIMAGE','LISTOVERIMAGE','SELECTIMAGE','FOCUSIMAGE','TOOLTIPIMAGE'):
            idx, rect, newpos = self.parse_image_idx()
            if idx is None: return False
            self.pos = newpos
            node[prop.lower()+'_idx'] = idx
            if rect is not None: node[prop.lower()+'_rect'] = rect
            return True
        if prop == 'IMAGESRCRECT':
            v = self.parse_4ints()
            if v is None: return False
            node['image_src_rect'] = v; return True
        if prop in ('TOOLTIPMSG','TEXT','BTNTEXT'):
            v = self.parse_int()
            if v is None: return False
            node[prop.lower()+'_msg_idx'] = v; return True
        if prop == 'ACTIVE':
            v = self.parse_int()
            if v is None: return False
            node['active'] = (v != 0); return True
        if prop == 'MOVEABLE':
            v = self.parse_int()
            if v is None: return False
            node['movable'] = (v != 0); return True
        if prop == 'AUTOCLOSE':
            v = self.parse_int()
            if v is None: return False
            node['auto_close'] = (v != 0); return True
        if prop == 'ALPHA':
            v = self.parse_int()
            if v is None: return False
            node['alpha'] = v & 0xff; return True
        if prop == 'FONTIDX':
            v = self.parse_int()
            if v is None: return False
            node['font_idx'] = v; return True
        if prop == 'ID':
            t = self.next_tok()
            if t is None: return False
            node['id'] = t; return True
        if prop == 'FUNC':
            t = self.next_tok()
            if t is None: return False
            node['func'] = t; return True
        if prop in self.INT_PROPS:
            for _ in range(self.INT_PROPS[prop]):
                if self.parse_int() is None: return False
            return True
        if prop in self.BOOL_PROPS:
            if self.parse_int() is None: return False
            return True
        self.skip_eol(); return True

    def parse_node(self, type_name):
        node = {'type': type_name}
        t = self.next_tok()
        if t != '{': return node
        while True:
            t = self.next_tok()
            if t is None or t == '}': return node
            if t.startswith('$'):
                child = self.parse_node(t[1:])
                if 'children' not in node: node['children'] = []
                node['children'].append(child)
                continue
            if t.startswith('#'):
                self.apply_prop(node, t[1:])
                continue
            self.skip_eol()
        return node

    def parse_all(self):
        roots = []
        while True:
            t = self.next_tok()
            if t is None: break
            if t.startswith('$'):
                roots.append(self.parse_node(t[1:]))
            else:
                self.skip_eol()
        return roots


count = 0
errors = []
for fname in sorted(os.listdir(src_dir)):
    if not fname.endswith('.bin'): continue
    full = os.path.join(src_dir, fname)
    try:
        with open(full, 'rb') as f:
            raw = f.read()
        ver, typ, fsz = struct.unpack('<III', raw[:12])
        payload = raw[13:13+fsz]
        plain = decrypt(payload, typ)
        text = plain.decode('latin-1')
        p = Parser(text)
        roots = p.parse_all()
        out_name = fname.replace('.bin', '.json')
        with open(os.path.join(out_dir, out_name), 'w', encoding='utf-8') as f:
            json.dump({'file': fname, 'roots': roots}, f, indent=1, ensure_ascii=False)
        count += 1
    except Exception as e:
        errors.append((fname, str(e)))

print(f'Wrote {count}')
print(f'Errors: {len(errors)}')
for f, e in errors[:5]:
    print(f'  {f}: {e}')
