/*
 * game.c — The King of Fighters 2000 (NGH-257) ROM set, shared by the player
 * and the generator so both load the program identically.
 *
 * KOF2000 ships with encrypted program, graphics and audio (NEO-SMA +
 * NEO-CMC50). The program here is the de-protected "kof2000n" layout
 * (257-pg1.p1 fixed + 257-pg2.sp2 banked), and the C/S/M1 files are the
 * CMC50-decrypted output of tools/decrypt.c. All are byte-swapped P ROMs,
 * so `swap` is 1.
 */
#include "game.h"

const ng_game_t k_kof2000 = {
    .name = "kof2000",
    .title = "The King of Fighters 2000",
    .prom_size = 0x500000,           /* 1 MB fixed + 4 MB banked */
    .prom = {
        {"257-pg1.p1", 0x000000, 0x000000, 0x100000, 1},
        {"257-pg2.sp2", 0x000000, 0x100000, 0x400000, 1},
    },
    .crom_size = 0x4000000,          /* 64 MB after pairing */
    .crom_pairs = {
        {"257-c1.c1", "257-c2.c2"},
        {"257-c3.c3", "257-c4.c4"},
        {"257-c5.c5", "257-c6.c6"},
        {"257-c7.c7", "257-c8.c8"},
    },
    .srom = "257-s1.s1",
    .m1 = "257-m1.m1",
    .vrom = {"257-v1.v1", "257-v2.v2", "257-v3.v3", "257-v4.v4"},
    .fix_bank_type = NG_FIX_BANK_KOF2000,  /* 512 KB S ROM with VRAM bank bits */
};
