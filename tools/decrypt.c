/*
 * decrypt.c — KOF2000 (NEO-CMC50) ROM preparation tool.
 *
 * Reads the encrypted MAME "kof2000" set and writes a self-contained,
 * runtime-ready ROM directory:
 *   - 257-m1.m1    CMC50-decrypted Z80 program
 *   - 257-c1..c8   CMC50-decrypted sprite ROMs (odd/even pairs preserved)
 *   - 257-s1.s1    fixed-layer ROM, extracted from the end of the C data
 *   - 257-pg1.p1 / 257-pg2.sp2  (already decrypted program; copied)
 *   - 257-v1..v4, and the MVS BIOS set (copied)
 *
 * The decryption algorithms and tables are ported from MAME's Neo Geo
 * driver (BSD-3-Clause; S.Smith, D.Haywood, F.Priuli) — see tools/prot_cmc.cpp.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "tables.h"

static uint16_t bitswap16(uint16_t val, const uint8_t *bits) {
    uint16_t r = 0;
    for (int i = 0; i < 16; i++)
        r |= (uint16_t)(((val >> bits[i]) & 1) << (15 - i));
    return r;
}

static uint8_t *read_file(const char *dir, const char *name, size_t *size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "decrypt: cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return NULL; }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

static int write_file(const char *dir, const char *name, const uint8_t *data, size_t size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "decrypt: cannot write %s\n", path); return -1; }
    if (fwrite(data, 1, size, f) != size) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

static int copy_file(const char *dir, const char *outdir, const char *name) {
    size_t size;
    uint8_t *d = read_file(dir, name, &size);
    if (!d) return -1;
    int rc = write_file(outdir, name, d, size);
    free(d);
    return rc;
}

/* ---- CMC50 graphics decryption ---- */

static void gfx_byte(uint8_t *r0, uint8_t *r1, uint8_t c0, uint8_t c1,
                     const uint8_t *t0hi, const uint8_t *t0lo, const uint8_t *t1,
                     uint32_t base, int invert) {
    uint8_t tmp = t1[(base & 0xff) ^ kof2000_address_0_7_xor[(base >> 8) & 0xff]];
    uint8_t xor0 = (uint8_t)((t0hi[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01));
    uint8_t xor1 = (uint8_t)((tmp & 0xfe) | (t0lo[(base >> 8) & 0xff] & 0x01));
    if (invert) { *r0 = c1 ^ xor0; *r1 = c0 ^ xor1; }
    else        { *r0 = c0 ^ xor0; *r1 = c1 ^ xor1; }
}

static void gfx_decrypt(uint8_t *rom, uint32_t rom_size) {
    uint8_t *buf = (uint8_t *)malloc(rom_size);
    const uint8_t *a07  = kof2000_address_0_7_xor;
    const uint8_t *a811 = kof2000_address_8_15_xor1;
    const uint8_t *a812 = kof2000_address_8_15_xor2;
    const uint8_t *a161 = kof2000_address_16_23_xor1;
    const uint8_t *a162 = kof2000_address_16_23_xor2;

    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        gfx_byte(&buf[4*rpos], &buf[4*rpos+3], rom[4*rpos], rom[4*rpos+3],
                 kof2000_type0_t03, kof2000_type0_t12, kof2000_type1_t03, rpos, (int)((rpos >> 8) & 1));
        gfx_byte(&buf[4*rpos+1], &buf[4*rpos+2], rom[4*rpos+1], rom[4*rpos+2],
                 kof2000_type0_t12, kof2000_type0_t03, kof2000_type1_t12, rpos,
                 (int)(((rpos >> 16) ^ a162[(rpos >> 8) & 0xff]) & 1));
    }
    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        uint32_t baser = rpos;
        baser ^= 0x00u; /* KOF2000_GFX_KEY */
        baser ^= (uint32_t)a811[(baser >> 16) & 0xff] << 8;
        baser ^= (uint32_t)a812[baser & 0xff] << 8;
        baser ^= (uint32_t)a161[baser & 0xff] << 16;
        baser ^= (uint32_t)a162[(baser >> 8) & 0xff] << 16;
        baser ^= a07[(baser >> 8) & 0xff];
        baser &= (rom_size / 4) - 1;
        rom[4*rpos]   = buf[4*baser];
        rom[4*rpos+1] = buf[4*baser+1];
        rom[4*rpos+2] = buf[4*baser+2];
        rom[4*rpos+3] = buf[4*baser+3];
    }
    free(buf);
}

/* ---- fixed-layer extraction ---- */

static void sfix_extract(const uint8_t *rom, uint32_t rom_size, uint8_t *fixed, uint32_t fixed_size) {
    const uint8_t *src = rom + rom_size - fixed_size;
    for (uint32_t i = 0; i < fixed_size; i++)
        fixed[i] = src[(i & ~0x1fu) + ((i & 7) << 2) + ((~i & 8) >> 2) + ((i & 0x10) >> 4)];
}

/* ---- CMC50 M1 decryption ---- */

static uint16_t sum16(const uint8_t *data, uint32_t length) {
    uint16_t sum = 0;
    for (uint32_t i = 0; i < length; i++) sum = (uint16_t)(sum + data[i]);
    return sum;
}

static uint32_t m1_scramble(uint32_t address, uint16_t key) {
    static const uint8_t k1[16] = {12,0,2,4,8,15,7,13,10,1,3,6,11,9,14,5};
    static const uint8_t k3[16] = {7,15,14,6,5,13,12,4,11,3,10,2,9,1,8,0};
    uint32_t block = (address >> 16) & 7;
    uint32_t aux = address & 0xffff;
    uint8_t p[16];
    for (int i = 0; i < 16; i++) p[i] = (uint8_t)p1[block][15 - i];
    aux ^= bitswap16(key, k1);
    aux = bitswap16((uint16_t)aux, p);
    aux ^= m1_address_0_7_xor[(aux >> 8) & 0xff];
    aux ^= (uint32_t)m1_address_8_15_xor[aux & 0xff] << 8;
    aux = bitswap16((uint16_t)aux, k3);
    return (block << 16) | aux;
}

static int decrypt_m1(const char *dir, const char *outdir) {
    size_t size;
    uint8_t *m1 = read_file(dir, "257-m1.m1", &size);
    if (!m1) return -1;
    if (size != 0x40000) { fprintf(stderr, "decrypt: 257-m1.m1 is %zu bytes, expected 0x40000\n", size); free(m1); return -1; }
    uint8_t *crypt = (uint8_t *)calloc(1, 0x80000);
    memcpy(crypt, m1, 0x40000);
    uint16_t key = sum16(crypt, 0x10000);
    uint8_t *buf = (uint8_t *)malloc(0x80000);
    for (uint32_t i = 0; i < 0x80000; i++)
        buf[i] = crypt[m1_scramble(i, key)];
    int rc = write_file(outdir, "257-m1.m1", buf, 0x40000);
    fprintf(stderr, "decrypt: M1 key=%04x -> 257-m1.m1 (0x40000 bytes)\n", key);
    free(buf); free(crypt); free(m1);
    return rc;
}

static int decrypt_crom(const char *dir, const char *outdir) {
    static const char *pairs[4][2] = {
        {"257-c1.c1", "257-c2.c2"}, {"257-c3.c3", "257-c4.c4"},
        {"257-c5.c5", "257-c6.c6"}, {"257-c7.c7", "257-c8.c8"},
    };
    size_t csize = 0;
    uint8_t *even[4], *odd[4];
    for (int i = 0; i < 4; i++) {
        even[i] = read_file(dir, pairs[i][0], &csize);
        odd[i]  = read_file(dir, pairs[i][1], &csize);
        if (!even[i] || !odd[i]) return -1;
    }
    uint32_t total = (uint32_t)csize * 2 * 4;  /* 64 MB */
    uint8_t *crom = (uint8_t *)malloc(total);
    for (int i = 0; i < 4; i++) {
        uint32_t base = (uint32_t)csize * 2 * i;
        for (uint32_t k = 0; k < csize; k++) {
            crom[base + 2*k]     = even[i][k];
            crom[base + 2*k + 1] = odd[i][k];
        }
    }
    gfx_decrypt(crom, total);

    /* S ROM: fixed layer, 0x80000 bytes at the end of the decrypted C data. */
    uint8_t *srom = (uint8_t *)malloc(0x80000);
    sfix_extract(crom, total, srom, 0x80000);
    if (write_file(outdir, "257-s1.s1", srom, 0x80000)) return -1;
    free(srom);
    fprintf(stderr, "decrypt: S ROM -> 257-s1.s1 (0x80000 bytes)\n");

    for (int i = 0; i < 4; i++) {
        uint32_t base = (uint32_t)csize * 2 * i;
        for (uint32_t k = 0; k < csize; k++) {
            even[i][k] = crom[base + 2*k];
            odd[i][k]  = crom[base + 2*k + 1];
        }
        if (write_file(outdir, pairs[i][0], even[i], csize)) return -1;
        if (write_file(outdir, pairs[i][1], odd[i],  csize)) return -1;
        free(even[i]); free(odd[i]);
    }
    free(crom);
    fprintf(stderr, "decrypt: C ROMs -> 257-c1..c8 (%zu bytes each)\n", csize);
    return 0;
}

int main(int argc, char **argv) {
    const char *romdir = "roms-src", *outdir = "roms";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom-path") && i + 1 < argc) romdir = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            fprintf(stderr, "usage: kof2000_decrypt [--rom-path DIR] [--out DIR]\n");
            return 0;
        }
    }
    if (decrypt_m1(romdir, outdir)) return 1;
    if (decrypt_crom(romdir, outdir)) return 1;
    const char *copy[] = {"257-pg1.p1", "257-pg2.sp2",
                          "257-v1.v1", "257-v2.v2", "257-v3.v3", "257-v4.v4",
                          "sp-s2.sp1", "sfix.sfix", "sm1.sm1", "000-lo.lo", NULL};
    for (int i = 0; copy[i]; i++)
        if (copy_file(romdir, outdir, copy[i])) return 1;
    fprintf(stderr, "decrypt: wrote a complete ROM set to %s\n", outdir);
    return 0;
}
