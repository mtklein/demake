"""Band manager's desk for the Kind of Azure combo session: validate the four
players' takes (lengths, ranges, notated-hit placement), splice, and write
full.json for press/emit. Kept as the exemplar for future multi-player
sessions -- section map and ensemble hits are Azure-specific."""
import sys, re
sys.path.insert(0, 'tools/music')
from songs import seq, rep, n, HOLD, REST

SECTIONS = [('INTRO',24),('HEAD',384),('SOLO_TPT',384),('SOLO_SAX',384),('OUT',192)]
ROLES = {'tpt':0,'sax':1,'bass':2,'drums':3}
RANGES = {'tpt':(n('G3'),n('E5')), 'sax':(n('C3'),n('B4')), 'bass':(n('D2'),n('G3'))}

def load(path):
    src = open(path).read()
    m = re.search(r'```(?:python)?\n(.*?)```', src, re.S)
    if m: src = m.group(1)
    ns = {'seq':seq,'rep':rep,'HOLD':HOLD,'REST':REST}
    exec(compile(src,path,'exec'), ns)
    return ns

def fix_len(name, sec, lst, want):
    if len(lst) < want:
        print(f"  {name}.{sec}: short {len(lst)}, padding {want-len(lst)} rest")
        lst = lst + [REST]*(want-len(lst))
    elif len(lst) > want:
        print(f"  {name}.{sec}: long {len(lst)}, trimming {len(lst)-want}")
        lst = lst[:want]
    return lst

def hits_ok(part, role):
    """check quartal stabs in HEAD A sections + OUT A."""
    lo = {'tpt':('D4','E4','D#4','F4'), 'sax':('A3','B3','A#3','C4')}[role]
    errs = []
    H = part['HEAD']; O = part['OUT']
    for s, eb in ((0,0),(96,0),(192,1),(288,0)):
        for k in (2,4,6,8):
            b = s+(k-1)*12
            w1, w2 = (n(lo[2]),n(lo[3])) if eb else (n(lo[0]),n(lo[1]))
            if H[b+9] != w1 or H[b+11] != w2:
                errs.append(('HEAD',s,k)); H[b+9],H[b+10],H[b+11] = w1,HOLD,w2
            nxt = b+12
            if nxt < len(H) and H[nxt] not in (REST,) and H[nxt] > 59:
                pass
    for k in (2,4,6,8):
        b = (k-1)*12
        if O[b+9] != n(lo[0]) or O[b+11] != n(lo[1]):
            errs.append(('OUT',0,k)); O[b+9],O[b+10],O[b+11] = n(lo[0]),HOLD,n(lo[1])
    return errs

parts = {}
for role in ROLES:
    p = load(f'{sys.argv[1]}/{role}.py')
    part = {}
    for sec, want in SECTIONS:
        assert sec in p, f"{role} missing {sec}"
        raw = [n(v) if isinstance(v, str) else v for v in p[sec]]
        part[sec] = fix_len(role, sec, raw, want)
    parts[role] = part

for role,(lo,hi) in RANGES.items():
    bad = [(sec,i,v) for sec,_ in SECTIONS for i,v in enumerate(parts[role][sec])
           if v <= 59 and not (lo <= v <= hi)]
    if bad: print(f"  {role}: {len(bad)} out-of-range notes, e.g. {bad[:4]} -- clamping")
    for sec,i,v in bad:
        parts[role][sec][i] = max(lo, min(hi, v))
for sec,_ in SECTIONS:
    bad = [v for v in parts['drums'][sec] if v not in (0,1,2,3,4,HOLD,REST)]
    assert not bad, f"drums bad tokens {bad[:5]}"

for role in ('tpt','sax'):
    errs = hits_ok(parts[role], role)
    if errs: print(f"  {role}: {len(errs)} hit placements corrected: {errs[:6]}")

full = {r: sum((parts[r][s] for s,_ in SECTIONS), []) for r in ROLES}
L = {r: len(v) for r,v in full.items()}
assert set(L.values()) == {1368}, L
print("parts assembled:", L)
import json
json.dump(full, open(f'{sys.argv[1]}/full.json','w'))
print("wrote full.json")
