/*
 * main.c — the KOF2000 player.
 *
 * Hands the runtime the ROM set and the table of recompiled routines that
 * kof2000_recomp generated from the user's own dump at build time. Without
 * that table (no ROMs configured) the runtime interprets.
 */
#include "game.h"

#ifdef KOF2000_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#endif

int main(int argc, char **argv) {
#ifdef KOF2000_HAVE_RECOMP
    return ng_main(argc, argv, &k_kof2000, ng_recomp_table, ng_recomp_count);
#else
    return ng_main(argc, argv, &k_kof2000, NULL, 0);
#endif
}
