Source release for the native static recompilation of **The King of Fighters 2000** (Neo Geo / NGH-257).

## No game data

This release contains **source code only**. It does not include ROMs, system ROMs (BIOS), the recompiled C, entry-point lists, executables or AppImages. Those are derived from the game and are generated locally from your own dump.

## Verified

Built and run with the Asian/European MVS BIOS (`sp-s2.sp1`):

- Boot: BIOS -> eye-catcher -> title -> character select -> match.
- 40,194 entry points; 2,465,472 native blocks, 76 interpreted instructions, 0 uncovered entry points in the scripted session.
- `--verify`: 2,465,444 blocks checked, 0 mismatches.
- Audio: NEO-CMC50 M1 decryption byte-exact; full BGM/SFX verified from boot through the match.
- Windows x64 (MSVC) and Linux x86_64 (gcc) builds.

The 76 interpreted instructions are a routine the game copies into work RAM and calls there; they are not reachable as ROM entry points.

## Fixed for this title (generic toolkit changes)

- MVS watchdog: KOF2000's copy protection waits for the board reset during a delay loop.
- Banked 512 KB fixed layer (NEO-CMC50): the top tile-code bits come from VRAM `$7500`.

The changes are in `patches/neogeorecomp-kof2000.patch` and are applied to the pinned toolkit submodule by `setup.sh`.

## Audio

The M1 driver's banked M-ROM reads now use the ROM's page layout directly. The
song tables at `$8CC1`/`$C5BE` and their sequences are read from the same pages
as MAME, the command/event queues stay drained, and the BGM plays through the
match. No audio-specific patch hunk is needed; the correct read path is the
toolkit's default one.

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
