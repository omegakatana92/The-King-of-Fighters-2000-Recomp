#!/bin/sh
# Build a Linux/Steam Deck AppImage from your own ROM dump.
#
# IMPORTANT: the resulting AppImage contains the statically recompiled game,
# which is code derived from the game ROMs. Keep it for your own use; do not
# redistribute it. The repository itself contains no game data.
#
# Requirements: cmake, a C/C++ compiler, SDL2 development files, appimagetool.
# Usage:
#   KOF2000_ROM_DIR=/path/to/roms APPIMAGETOOL=/path/to/appimagetool \
#       scripts/build-appimage.sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
: "${KOF2000_ROM_DIR:?set KOF2000_ROM_DIR to the folder with the kof2000 + neogeo ROM files}"
appimagetool=${APPIMAGETOOL:-appimagetool}
build=${BUILD_DIR:-$root/build-linux}
appdir=$build/AppDir
out=${OUT:-$root/dist/KOF2000-x86_64.AppImage}

"$root/setup.sh"

cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release \
    -DKOF2000_ROM_DIR="$KOF2000_ROM_DIR"
cmake --build "$build" -j"$(nproc 2>/dev/null || echo 4)"

bin=$build/KOF2000
[ -x "$bin" ] || { echo "build did not produce $bin" >&2; exit 1; }

rm -rf "$appdir"
mkdir -p "$appdir/usr/bin" "$appdir/usr/lib" \
    "$appdir/usr/share/applications" \
    "$appdir/usr/share/icons/hicolor/256x256/apps"
cp "$bin" "$appdir/usr/bin/KOF2000"
cp "$build/kof2000_decrypt" "$appdir/usr/bin/kof2000_decrypt"
cp "$root/assets/kof2000.png" "$appdir/usr/share/icons/hicolor/256x256/apps/kof2000.png"
cp "$root/assets/kof2000.png" "$appdir/kof2000.png"
cp "$root/packaging/kof2000.desktop" "$appdir/kof2000.desktop"

# Bundle what the Deck may not have at the right version, but never glibc.
for lib in $(ldd "$bin" | awk '/=>/ {print $3}' | grep -E 'lib(SDL2|stdc\+\+|gcc_s)'); do
    [ -f "$lib" ] && cp -L "$lib" "$appdir/usr/lib/"
done

cat > "$appdir/AppRun" <<'EOF'
#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export LD_LIBRARY_PATH="$here/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
data=${XDG_DATA_HOME:-$HOME/.local/share}/kof2000
roms=$data/roms

if [ ! -f "$roms/.decrypted" ]; then
    src=${KOF2000_ROM_DIR:-}
    if [ -z "$src" ] || [ ! -d "$src" ]; then
        title="Select the folder with the KOF2000 ROMs and Neo Geo BIOS files"
        if command -v zenity >/dev/null 2>&1; then
            src=$(zenity --file-selection --directory --title="$title") || exit 1
        elif command -v kdialog >/dev/null 2>&1; then
            src=$(kdialog --getexistingdirectory "$HOME" --title "$title") || exit 1
        else
            echo "Set KOF2000_ROM_DIR to the ROM folder and run again." >&2
            exit 1
        fi
    fi
    mkdir -p "$roms"
    "$here/usr/bin/kof2000_decrypt" --rom-path "$src" --out "$roms"
    touch "$roms/.decrypted"
fi

exec "$here/usr/bin/KOF2000" --rom-path "$roms" --bios-path "$roms" "$@"
EOF
chmod +x "$appdir/AppRun"

mkdir -p "$(dirname "$out")"
"$appimagetool" --no-appstream "$appdir" "$out"
echo "built $out"
