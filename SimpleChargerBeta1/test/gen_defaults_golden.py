#!/usr/bin/env python3
"""Genererar test/golden_defaults.inc fran SmedbyCharger_v2.2.ini.

Oberoende kontroll av shared/eepromDefaults.cpp: standardvardena raknas har
fram direkt ur ini-filens [Constants] och defaultValue-rader (raw = varde /
skala - translate, bits = index i alternativlistan, falt utan defaultValue = 0).

Anvandning:  python3 gen_defaults_golden.py <SmedbyCharger_v2.2.ini> > golden_defaults.inc
(ini-filen ligger i LTT-Swedens Drive-mapp, inte i repot.)
"""
import re
import sys

SIZES = {1: 38, 2: 84, 3: 38, 4: 68, 5: 64}
TYPES = {"U08": (1, False), "S08": (1, True), "U16": (2, False), "S16": (2, True)}


def split_fields(s):
    return [f.strip() for f in re.findall(r'"[^"]*"|[^,]+', s)]


def main(path):
    lines = open(path, encoding="latin-1").read().splitlines()
    section, page = None, None
    fields, defaults = {}, {}
    for raw in lines:
        line = raw.split(";", 1)[0].strip() if not raw.strip().startswith(";") else ""
        if not line:
            continue
        m = re.match(r"\[(\w+)\]", line)
        if m:
            section = m.group(1)
            continue
        if section == "Constants":
            m = re.match(r"page\s*=\s*(\d+)", line)
            if m:
                page = int(m.group(1))
                continue
            m = re.match(r"(\w+)\s*=\s*(scalar|bits|array)\s*,(.*)", line)
            if m and page in SIZES:
                fields[m.group(1)] = (page, m.group(2), split_fields(m.group(3)))
        elif section == "ConstantsExtensions":
            m = re.match(r"defaultValue\s*=\s*(\w+)\s*,\s*(.*)", line)
            if m:
                defaults[m.group(1)] = m.group(2).strip()

    pages = {p: bytearray(n) for p, n in SIZES.items()}
    for name, (page, kind, f) in fields.items():
        typ, off = f[0], int(f[1])
        size, signed = TYPES[typ]
        if name not in defaults:
            continue  # ingen defaultValue -> 0
        dv = defaults[name]
        if kind == "bits":
            lo, hi = [int(x) for x in re.match(r"\[(\d+):(\d+)\]", f[2]).groups()]
            opts = [o.strip('"') for o in f[3:]]
            idx = opts.index(dv.strip('"'))
            assert idx < (1 << (hi - lo + 1)), name
            pages[page][off] |= idx << lo
        elif kind == "scalar":
            scale, translate = float(f[3]), float(f[4])
            rawv = int(round(float(dv) / scale - translate))
            pages[page][off:off + size] = rawv.to_bytes(size, "little", signed=signed)
        else:
            raise SystemExit("array-default stods inte: " + name)
    for name in defaults:
        assert name in fields, "defaultValue for okant falt: " + name
    out = []
    for p in sorted(pages):
        out.append("/* sida %d (%d B) */" % (p, SIZES[p]))
        b = pages[p]
        for i in range(0, len(b), 16):
            out.append(" ".join("0x%02X," % x for x in b[i:i + 16]))
    print("/* Genererad av gen_defaults_golden.py fran SmedbyCharger_v2.2.ini - andra inte for hand. */")
    print("\n".join(out))


if __name__ == "__main__":
    main(sys.argv[1])
