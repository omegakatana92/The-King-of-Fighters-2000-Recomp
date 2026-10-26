# The King of Fighters 2000 (Neo Geo) - static recompilation

A native recompilation of *The King of Fighters 2000* (SNK / NGH-257), built on
the [neogeorecomp](https://github.com/sp00nznet/neogeorecomp) toolkit. The
game's Motorola 68000 program is translated ahead of time into C and runs
natively against a model of the MVS board; the Z80 sound driver and YM2610 are
emulated.

> **No game data is stored in this repository.** ROMs, system ROMs (BIOS) and
> anything generated from them - the recompiled C, entry-point lists and any
> executable or AppImage - are not committed. You must own a KOF2000 dump; the
> decryption and recompilation run locally and write into your build directory.
> The playable binary contains code derived from the game and must not be
> redistributed without the rights holder's permission.

## Status

Verified on Windows x64 (MSVC) and Linux x86_64 (gcc) with the Asian/European
MVS BIOS (`sp-s2.sp1`). "Verified" means it was actually run; the coverage
figures are from a 4,000-frame scripted session and are not a claim about every
code path.

| Area | State |
|---|---|
| Boot | Verified: BIOS -> eye-catcher -> title -> character select -> match |
| Program | De-protected "kof2000n" layout, 1 MB fixed + 4 MB banked |
| Graphics | NEO-CMC50 C-ROM + 512 KB S-ROM decryption; banked fixed layer; verified in screenshots |
| Audio | NEO-CMC50 M1 decryption byte-exact; boot/title/select audio works. **Known issue:** stage/match BGM and SFX go silent (see Known issues). |
| Native recompilation | 40,194 entry points; 2,465,472 native blocks, 76 interpreted instructions, 0 uncovered entry points |
| `--verify` | 2,465,444 blocks checked, 0 mismatches |
| MVS watchdog | Implemented as a generic toolkit fix (KOF2000's copy protection waits for it) |
| Window | Built and run with SDL2 on Windows; Linux build uses SDL2 as well |
| Platform | Windows x64 and Linux x86_64; Steam Deck AppImage build script included |

The 76 interpreted instructions are a small routine the game copies into work
RAM and calls there; they are not reachable as ROM entry points, so the
recompiler cannot seed them.

## Known issues

- **Stage/match audio is silent.** Boot, title and character-select audio play,
  but when a match starts the M1 driver stops draining its event/command
  queues and gets stuck in the sequence player (the ADPCM/FM registers are
  left cleared). The CMC50 M1 data is
  byte-exact, the Z80 bank base matches MAME, and forced FM/ADPCM key-ons prove
  the YM2610/V-ROM path works. The remaining fault is in the driver's
  command/sequence state, not the decryption or the synthesizer. Visuals and
  gameplay are unaffected.

## How it works

KOF2000 ships with three layers of encryption:

1. **Program (NEO-SMA).** The original set stores the 68k code scrambled.
   The de-protected `kof2000n` set (`257-pg1.p1` + `257-pg2.sp2`) contains the
   same code already linearized, so this project uses that layout directly and
   needs no program decryption.
2. **Graphics (NEO-CMC50).** The eight C ROMs and the fixed layer (stored at
   the end of the C data) are CMC50-encrypted. `tools/decrypt.c` decrypts them.
3. **Audio (NEO-CMC50).** `257-m1.m1` is CMC50-encrypted; `tools/decrypt.c`
   decrypts it using the checksum-derived key.

`tools/decrypt.c` runs at build time and writes a complete, runtime-ready ROM
directory. The decryption algorithms and tables are ported from MAME's Neo Geo
driver (BSD-3-Clause; S.Smith, D.Haywood, F.Priuli) - see `tools/prot_cmc.cpp`.

Two runtime details matter for this title and are implemented generically in
the toolkit, not as title-specific hacks:

* **Watchdog.** KOF2000's copy protection runs a delay loop and expects the
  MVS watchdog to reset the board during it. Without the reset the game writes
  a failure flag to backup RAM and shows the "original SNK boards" warning
  forever. The runtime resets after 3,244,030 master clocks (about 0.135 s)
  without a kick to `$300001`, preserving work and backup RAM.
* **Banked fixed layer.** A 512 KB S ROM cannot address all its tiles with the
  12 code bits in a fix-map entry. `ng_game_t.fix_bank_type` selects the
  KOF2000 layout, where the top two code bits come from VRAM at `$7500`; carts
  with a 128 KB S ROM keep the plain path.

Both changes, plus the Z80 M-ROM bank base correction, are in
`patches/neogeorecomp-kof2000.patch` and are applied to the pinned toolkit
submodule by `setup.sh`.

## Requirements

* Git, CMake 3.21+, a C/C++ compiler (MSVC 2022+ on Windows, gcc/clang on
  Linux), and SDL2 development files for a windowed build.
* Your own ROM files (nothing is committed or downloaded):

```
kof2000/                     the MAME "kof2000" set, unzipped
  257-c1.c1 ... 257-c8.c8    8 MB each (encrypted C ROMs)
  257-m1.m1                  256 KB (encrypted Z80 program)
  257-p1.p1, 257-p2.p2       4 MB each (encrypted program - unused)
  257-pg1.p1                 1 MB  (de-protected fixed program)
  257-pg2.sp2                4 MB  (de-protected banked program)
  257-v1.v1 ... 257-v4.v4    4 MB each (ADPCM samples)
neogeo/                      the MVS system ROM set
  sp-s2.sp1  sfix.sfix  sm1.sm1  000-lo.lo
```

## Build

```sh
git clone --recursive https://github.com/omegakatana92/The-King-of-Fighters-2000-Recomp.git
cd The-King-of-Fighters-2000-Recomp
./setup.sh
```

`setup.sh` initialises the toolkit submodule and applies the generic toolkit
patch. Then configure with the folder that contains both the game and BIOS
files:

**Windows (MSVC)**

```bat
cmake -S . -B build -A x64 -DKOF2000_ROM_DIR=C:/path/to/roms
cmake --build build --config Release
```

**Linux**

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DKOF2000_ROM_DIR="$HOME/roms"
cmake --build build -j
```

The build decrypts, recompiles and links `KOF2000.exe` on Windows or
`build/KOF2000` on Linux. Without `KOF2000_ROM_DIR` the player builds
interpreter-only (useful for bring-up, not for play).

The generated routines are enormous dispatch functions. Their optimizer cost
is superlinear: the build compiles them at `/O1` (MSVC) or `-O1` (gcc/clang),
which takes seconds per file. Do not raise that to `/O2`/`-O2` without
measuring.

## Run

```
KOF2000 --rom-path build/roms --bios-path build/roms
```

Options are the standard neogeorecomp set (`--headless`, `--interp`, `--verify`,
`--dump-misses`, `--screenshot`, `--input`, `--frames`, ...); see
`neogeorecomp/docs/running.md`. The decrypted ROM set is written to
`build/roms` during the build.

## Steam Deck AppImage

`scripts/build-appimage.sh` builds a Linux x86_64 AppImage for Steam Deck (or
any Linux desktop). It needs `appimagetool` and SDL2 development files, and
takes your ROM folder:

```sh
KOF2000_ROM_DIR="$HOME/roms" APPIMAGETOOL=/path/to/appimagetool \
    scripts/build-appimage.sh
```

The image bundles the player, the decryption tool, SDL2 and an icon. On first
launch it asks for the ROM folder, decrypts into
`~/.local/share/kof2000/roms`, and then starts the game; the ROMs are never
stored inside the AppImage.

**Do not redistribute the resulting AppImage.** It contains the statically
recompiled game, which is derived from the ROMs. This repository only
distributes the source and the generic toolkit fixes.

## Testing

```sh
# boot, insert a coin, pick a team, fight
build/KOF2000 --headless --rom-path build/roms --bios-path build/roms \
    --frames 4000 --input tests/coin_start.txt

# replay every native block in the interpreter and compare
build/KOF2000 --headless --verify --rom-path build/roms \
    --bios-path build/roms --frames 4000 --input tests/coin_start.txt

# refresh runtime-only entry points, then rebuild
cmake --build build --target profile
cmake --build build
```

The `profile` target runs a 6,000-frame attract session and a 19,000-frame
scripted session, merges the addresses it had to interpret into
`build/entries.txt`, and the next build regenerates the table with them. It is
not committed (it is derived from the ROMs).

## Controls

Keyboard: arrows, **Z X C V** = A B C D, **5** = coin, **1** = start (P1),
**2** = start (P2), **3** = select, **9** = service, **F2** = test, **Esc** =
quit. Gamepads work through SDL2. On an MVS, insert a coin before Start.

## Layout

```
src/game.c, game.h     the ng_game_t ROM layout
src/main.c             the player (ng_main)
src/recomp_main.c      the generator (ng_recomp_main)
tools/decrypt.c        NEO-CMC50 decryption (build-time)
tools/tables.h         CMC50 tables, generated from MAME's prot_cmc.cpp
tools/prot_cmc.cpp     MAME's CMC code, for reference and table generation
tools/gen_tables.py    regenerates tools/tables.h
tests/coin_start.txt   headless input script
patches/               generic neogeorecomp fixes needed by this title
scripts/               Linux/Steam Deck AppImage builder
packaging/             desktop entry for the AppImage
```

## License and third-party code

The repository is licensed under the GNU General Public License v2.0 (see
`LICENSE`). The original game glue and decryption tool are also available under
MIT; the toolkit is MIT, the MAME CMC code is BSD-3-Clause, and the bundled
libraries keep their own licenses. See `THIRD_PARTY.md`.

No ROMs, BIOS images or ROM-derived generated code are committed or
distributed in this repository or its releases.
