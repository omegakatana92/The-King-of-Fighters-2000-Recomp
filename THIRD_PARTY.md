# Third-party components and licenses

This repository is released under the GNU General Public License v2.0 (see
`LICENSE`). The original game glue in `src/` and the decryption tool are also
available under the terms noted below; the combined work is distributed under
the GPLv2.

| Component | Where | License |
|---|---|---|
| neogeorecomp toolkit | git submodule `neogeorecomp/` | MIT (see its `LICENSE`) |
| Musashi 68000 core | inside neogeorecomp | see `neogeorecomp/LICENSES` |
| Z80 core | inside neogeorecomp | see `neogeorecomp/LICENSES` |
| ymfm (YM2610) | inside neogeorecomp | BSD-3-Clause (see its headers) |
| SDL2 | linked/bundled in AppImage builds | zlib license |
| NEO-CMC50 decryption tables | `tools/prot_cmc.cpp` | BSD-3-Clause, copyright S. Smith, D. Haywood, F. Priuli (MAME) |
| Game glue (`src/`, `tools/decrypt.c`) | this repository | MIT |

No ROMs, system ROMs, or output generated from them (recompiled C, entry-point
lists, executables) are part of this repository. The AppImage produced by
`scripts/build-appimage.sh` contains recompiled game code and must not be
redistributed; it exists for the person who owns the ROM dump.
