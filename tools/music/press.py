"""Press a NEW track into songs.py: validate a 4-channel take file, emit the
song function, and register it in SONGS/ORDER. Run from the repo root:
usage: press.py <part_file.py> <NAME> <ch0,ch1,ch2,ch3 var names> <speed> <env1> <env2> <loop>"""
import sys, re
sys.path.insert(0, 'tools/music')
from songs import seq, rep, n, HOLD, REST
PC = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B']

path, NAME, chvars, speed, env1, env2, loop = sys.argv[1:8]
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
        if isinstance(a, str):        out += [_tok(t) for t in a.split()]
        elif isinstance(a, (list, tuple)): out += _flat(a)
        else:                          out.append(a)
    return out
def bseq(*args): return _flat(args)          # tokenizes space-separated bars
def brep(pattern, times): return _flat([pattern] * times)
ns = {'seq':bseq,'rep':brep,'HOLD':HOLD,'REST':REST,'n':n}
exec(compile(src, path, 'exec'), ns)

chans = []
for cn in chnames:
    assert cn in ns, f"missing {cn}"
    chans.append([n(v) if isinstance(v,str) else v for v in ns[cn]])
L = {len(c) for c in chans}
assert len(L) == 1, f"unequal lengths {[len(c) for c in chans]}"
rows = L.pop()
bad = [(r,v) for r,v in enumerate(chans[2]) if v <= 59 and v > 47]
assert not bad, f"wave too high: {bad[:5]}"
bad = [(r,v) for r,v in enumerate(chans[3]) if v not in (0,1,2,3,4,HOLD,REST)]
assert not bad, f"bad drums: {bad[:5]}"
for i in (0,1):
    bad = [(r,v) for r,v in enumerate(chans[i]) if not (0 <= v <= 61)]
    assert not bad, f"ch{i} out of range: {bad[:5]}"
print(f"{NAME}: {rows} rows x4 channels validated")

def tok(v):
    if v == HOLD: return '.'
    if v == REST: return 'x'
    return PC[v%12] + str(v//12+2)
def emit(name, data, drums=False):
    lines = [f"    {name} = ("]
    for a in range(0, rows, 64):
        b = min(a+64, rows)
        if drums:
            toks = ','.join(str(v) if v<=4 else ('HOLD' if v==HOLD else 'REST') for v in data[a:b])
            frag = f"        [{toks}]"
        else:
            frag = f"        bars('{' '.join(tok(v) for v in data[a:b])}')"
        lines.append(frag + (" +" if b < rows else ""))
    lines.append("    )")
    return '\n'.join(lines)

# header comment passed via file: first contiguous '#' lines of the part source
hdr = []
for line in src.splitlines():
    if line.startswith('#'): hdr.append(line)
    elif hdr: break
fn = NAME.lower()
block = "\n# " + "-"*60 + f" {NAME} (jukebox)\n"
block += '\n'.join(hdr) + f"\ndef {fn}():\n"
block += emit('c0', chans[0]) + '\n' + emit('c1', chans[1]) + '\n'
block += emit('c2', chans[2]) + '\n' + emit('c3', chans[3], True) + '\n'
block += f"""    return dict(ch=[c0, c1, c2, c3], loop={loop}, speed={speed},
                env1={env1}, env2={env2}, wavevol=0x2000)

SONGS['{NAME}'] = {fn}()
"""
s = open('tools/music/songs.py').read()
mo = re.search(r"ORDER = \[([^\]]*)\]", s)
assert mo and f"'{NAME}'" not in mo.group(1), "already registered?"
s = s.replace(mo.group(0), block + "\n" + f"ORDER = [{mo.group(1)}, '{NAME}']", 1)
open('tools/music/songs.py','w').write(s)
print(f"{NAME} pressed into songs.py; ORDER extended")
