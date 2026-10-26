#!/usr/bin/env python3
# Extract CMC50/SMA decryption tables from MAME source and verify M1 decryption.
import re, sys, os

def parse_table(src, name):
    m = re.search(re.escape(name) + r'\s*\[\s*256\s*\]\s*=\s*\{(.*?)\};', src, re.S)
    assert m, "table not found: " + name
    body = m.group(1)
    vals = [int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]+)', body)]
    assert len(vals) == 256, "%s: got %d values" % (name, len(vals))
    return bytes(vals)

def parse_p1(src):
    m = re.search(r'const int p1\[8\]\[16\]\s*=\s*\{(.*?)\};', src, re.S)
    assert m, "p1 not found"
    body = m.group(1)
    rows = re.findall(r'\{([^}]*)\}', body)
    assert len(rows) == 8, "p1 rows: %d" % len(rows)
    return [[int(x) for x in re.findall(r'\b\d+\b', r)] for r in rows]

def bitswap(val, *bits):
    r = 0
    for i, b in enumerate(bits):
        r |= ((val >> b) & 1) << i
    return r

def main():
    proj = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    src = open(os.path.join(proj, "tools", "prot_cmc.cpp"), encoding="utf-8").read()

    T = {}
    for name in ["kof2000_type0_t03","kof2000_type0_t12","kof2000_type1_t03","kof2000_type1_t12",
                 "kof2000_address_8_15_xor1","kof2000_address_8_15_xor2",
                 "kof2000_address_16_23_xor1","kof2000_address_16_23_xor2",
                 "kof2000_address_0_7_xor","m1_address_8_15_xor","m1_address_0_7_xor"]:
        T[name] = parse_table(src, name)
    p1 = parse_p1(src)

    # verify against the tables I hand-transcribed in decrypt.py (skip; just use parsed)
    m1addr815 = T["m1_address_8_15_xor"]
    m1addr07 = T["m1_address_0_7_xor"]

    def sum16(data):
        s = 0
        for b in data:
            s = (s + b) & 0xffff
        return s

    def scramble(address, key):
        block = (address >> 16) & 7
        aux = address & 0xffff
        aux ^= bitswap(key, 12,0,2,4,8,15,7,13,10,1,3,6,11,9,14,5)
        aux = bitswap(aux, *[p1[block][15-i] for i in range(16)])
        aux ^= m1addr07[(aux >> 8) & 0xff]
        aux ^= m1addr815[aux & 0xff] << 8
        aux = bitswap(aux, 7,15,14,6,5,13,12,4,11,3,10,2,9,1,8,0)
        return (block << 16) | aux

    m1 = open(os.path.join(proj, "roms-src", "257-m1.m1"), "rb").read()
    crypt = m1 + bytes(0x40000)
    key = sum16(crypt[0:0x10000])
    buf = bytearray(0x80000)
    for i in range(0x80000):
        buf[i] = crypt[scramble(i, key)]
    dec = bytes(buf)
    oracle = open(os.path.join(proj, "roms-src", "257-m1.oracle.bin"), "rb").read()
    print("key=%04x" % key)
    print("dec[0:32]   =", dec[0:32].hex())
    print("oracle[0:32]=", oracle[0:32].hex())
    print("MATCH first 256KB:", dec[0:0x40000] == oracle)

if __name__ == "__main__":
    main()
