#!/usr/bin/env python3
# Emit tables.h (C arrays) from MAME's prot_cmc.cpp, for tools/decrypt.c.
import re, os

def main():
    proj = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    src = open(os.path.join(proj, "tools", "prot_cmc.cpp"), encoding="utf-8").read()

    def pt(name):
        m = re.search(re.escape(name) + r'\s*\[\s*256\s*\]\s*=\s*\{(.*?)\};', src, re.S)
        assert m, name
        return [int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]+)', m.group(1))]

    names = ["kof2000_type0_t03","kof2000_type0_t12","kof2000_type1_t03","kof2000_type1_t12",
             "kof2000_address_8_15_xor1","kof2000_address_8_15_xor2",
             "kof2000_address_16_23_xor1","kof2000_address_16_23_xor2",
             "kof2000_address_0_7_xor","m1_address_8_15_xor","m1_address_0_7_xor"]

    m = re.search(r'const int p1\[8\]\[16\]\s*=\s*\{(.*?)\};', src, re.S)
    p1 = [[int(x) for x in re.findall(r'\b\d+\b', r)] for r in re.findall(r'\{([^}]*)\}', m.group(1))]

    out = []
    out.append("/* Generated from MAME prot_cmc.cpp (BSD-3-Clause). Do not edit. */")
    out.append("#ifndef KOF2000_TABLES_H")
    out.append("#define KOF2000_TABLES_H")
    out.append("#include <stdint.h>")
    for n in names:
        vals = pt(n)
        out.append("static const uint8_t %s[256] = {" % n)
        for i in range(0, 256, 16):
            out.append("  " + ",".join("0x%02x" % v for v in vals[i:i+16]) + ",")
        out.append("};")
    out.append("static const int p1[8][16] = {")
    for r in p1:
        out.append("  {" + ",".join(str(v) for v in r) + "},")
    out.append("};")
    out.append("#endif")
    open(os.path.join(proj, "tools", "tables.h"), "w").write("\n".join(out) + "\n")
    print("wrote tables.h")

if __name__ == "__main__":
    main()
