"""RE-CUT an existing track: replace its function in songs.py in place
(ORDER and the jukebox are untouched). Run from the repo root:
usage: recut.py <part_file> <FUNC> <KEY> <ch0,ch1,ch2,ch3> <speed> <env1> <env2> <loop>"""
import sys, re
sys.path.insert(0, 'tools/music')
from songs import seq, rep, n, HOLD, REST
PC = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B']

path, FUNC, KEY, chvars, speed, env1, env2, loop = sys.argv[1:9]
chnames = chvars.split(',')
src = open(path).read()
m = re.search(r'```(?:python)?\n(.*?)```', src, re.S)
if m: src = m.group(1)
def _tok(t):
    if t in ('-', '.'): return HOLD
    if t in ('x', 'R'): return REST
    if t.lstrip('-').isdigit(): return int(t)
    return n(t)
def _flat(args):
    out = []
    for a in args:
        if isinstance(a, str):             out += [_tok(t) for t in a.split()]
        elif isinstance(a, (list, tuple)): out += _flat(a)
        else:                              out.append(a)
    return out
ns = {'seq':lambda *a:_flat(a), 'rep':lambda p,t:_flat([p]*t),
      'HOLD':HOLD, 'REST':REST, 'n':n}
exec(compile(src, path, 'exec'), ns)

chans = [ns[c] for c in chnames]
L = {len(c) for c in chans}
assert len(L) == 1, f"unequal lengths {[len(c) for c in chans]}"
rows = L.pop()
assert not [v for v in chans[2] if 47 < v <= 59], "wave note > B5"
assert not [v for v in chans[3] if v not in (0,1,2,3,4,HOLD,REST)], "bad drum token"
for i in (0,1):
    assert not [v for v in chans[i] if not (0 <= v <= 61)], f"ch{i} out of range"
print(f"{KEY}: {rows} rows x4 validated")

def tok(v):
    return '.' if v==HOLD else 'x' if v==REST else PC[v%12]+str(v//12+2)
def emit(name, data, drums=False):
    out = [f"    {name} = ("]
    for a in range(0, rows, 64):
        b = min(a+64, rows)
        if drums:
            frag = "        [" + ','.join(str(v) if v<=4 else ('HOLD' if v==HOLD else 'REST') for v in data[a:b]) + "]"
        else:
            frag = "        bars('" + ' '.join(tok(v) for v in data[a:b]) + "')"
        out.append(frag + (" +" if b < rows else ""))
    out.append("    )")
    return '\n'.join(out)

hdr = '\n'.join(l for l in src.splitlines()[:6] if l.startswith('#'))
body =  f"def {FUNC}():\n"
body += emit('c0', chans[0]) + '\n' + emit('c1', chans[1]) + '\n'
body += emit('c2', chans[2]) + '\n' + emit('c3', chans[3], True) + '\n'
body += (f"    return dict(ch=[c0, c1, c2, c3], loop={loop}, speed={speed},\n"
         f"                env1={env1}, env2={env2}, wavevol=0x2000)\n\n"
         f"SONGS['{KEY}'] = {FUNC}()")

s = open('tools/music/songs.py').read()
pat = re.compile(r"def %s\(\):.*?SONGS\['%s'\] = %s\(\)" % (FUNC, KEY, FUNC), re.S)
assert pat.search(s), f"could not find existing {FUNC}()/{KEY} block"
s = pat.sub(lambda _: (hdr + '\n' if hdr else '') + body, s, count=1)
open('tools/music/songs.py','w').write(s)
print(f"{KEY} recut in place ({rows} rows); ORDER/jukebox untouched")
