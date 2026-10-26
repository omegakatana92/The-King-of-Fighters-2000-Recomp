Source release for the native static recompilation of **The King of Fighters 2000** (Neo Geo / NGH-257).

## No game data

This release contains **source code only**. It does not include ROMs, system ROMs (BIOS), the recompiled C, entry-point lists, executables or AppImages. Those are derived from the game and are generated locally from your own dump.

## Verified

Built and run with the Asian/European MVS BIOS (`sp-s2.sp1`):

- Boot: BIOS -> eye-catcher -> title -> character select -> match.
- 40,194 entry points; 2,465,472 native blocks, 76 interpreted instructions, 0 uncovered entry points in the scripted session.
- `--verify`: 2,465,444 blocks checked, 0 mismatches.
- Audio: NEO-CMC50 M1 decryption byte-exact against the decrypted oracle; stereo output non-silent.
- Windows x64 (MSVC) and Linux x86_64 (gcc) builds.

The 76 interpreted instructions are a routine the game copies into work RAM and calls there; they are not reachable as ROM entry points.

## Fixed for this title (generic toolkit changes)

- MVS watchdog: KOF2000's copy protection waits for the board reset during a delay loop.
- Banked 512 KB fixed layer (NEO-CMC50): the top tile-code bits come from VRAM `$7500`.
- Z80 M-ROM bank base correction (`0x10000 + ...`, as in MAME's `init_audio`).

The changes are in `patches/neogeorecomp-kof2000.patch` and are applied to the pinned toolkit submodule by `setup.sh`.

## Known issue

- **Stage/match audio is silent.** Boot, title and character-select audio play, but when a match starts the M1 driver stops programming its ADPCM/FM channels (pan, level and sample registers are cleared). The CMC50 M1 data is byte-exact, the Z80 bank base matches MAME, and forced key-ons prove the YM2610/V-ROM path works; the remaining fault is in the driver's command/sequence state. Visuals and gameplay are unaffected.

## Build

```sh
git clone --recursive https://github.com/omegakatana92/The-King-of-Fighters-2000-Recomp.git
cd The-King-of-Fighters-2000-Recomp
./setup.sh
cmake -S . -B build -A x64 -DKOF2000_ROM_DIR=C:/path/to/roms   # Windows
cmake --build build --config Release
```

On Linux:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DKOF2000_ROM_DIR="$HOME/roms"
cmake --build build -j
```

See `README.md` for the expected ROM files, controls, profiling and testing.

## Steam Deck

`scripts/build-appimage.sh` builds a Linux x86_64 AppImage from your own ROMs. It is **not attached here**: the image contains the statically recompiled game, which is code derived from the ROMs. Keep it for your own use and do not redistribute it.

## License

Repository: GPLv2. Game glue and decryption: MIT. MAME CMC code: BSD-3-Clause. Toolkit: MIT. See `THIRD_PARTY.md`.
