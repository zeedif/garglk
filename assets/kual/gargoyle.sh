#!/bin/sh
# Starts Gargoyle from wherever its folder was copied (/mnt/us/gargoyle or extensions/gargoyle).
# Set GARGOYLE_FULLSCREEN=1 to start with the keyboard hidden, for external keyboards.

GARGOYLE_DIR="$(CDPATH='' cd "$(dirname "$0")" && pwd -P)"
DIST="${GARGOYLE_DIR}/dist"
LOG="${GARGOYLE_DIR}/gargoyle.log"

export DISPLAY="${DISPLAY:-:0}"
export GAMES="${GAMES:-${GARGOYLE_DIR}/games}"
export SAVED_GAMES="${SAVED_GAMES:-${GARGOYLE_DIR}/saved_games}"
export GTK2_RC_FILES="${DIST}/gtkrc"
export LD_LIBRARY_PATH="${DIST}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

mkdir -p "${GAMES}" "${SAVED_GAMES}"

# Older FBInk builds do not know the newest devices, so prefer the ones kept up to date
for FBINK in /var/local/kmc/bin/fbink /mnt/us/koreader/fbink "${DIST}/fbink"; do
    if [ -x "${FBINK}" ]; then
        "${FBINK}" -q -g file="${GARGOYLE_DIR}/gargoyle.png",halign=MIDDLE,valign=MIDDLE
        break
    fi
done

if [ -f "${LOG}" ]; then
    tail -c 100000 "${LOG}" >"${LOG}.new"
    mv -f "${LOG}.new" "${LOG}"
fi

cd "${DIST}" || exit 1
./gargoyle "$@" >>"${LOG}" 2>&1

lipc-set-prop -s com.lab126.keyboard close net.fabiszewski.gargoyle
