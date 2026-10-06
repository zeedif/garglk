#!/bin/sh
# Assembles the output of support/kindle/build.sh into build-kindlehf/package:
#   gargoyle-kindlehf.zip              gargoyle/ and documents/, to extract at the Kindle root
#   gargoyle_<version>_kindlehf.kpkg   the same files as a KPM package

set -e

TOP=$(CDPATH='' cd "$(dirname "$0")/../.." && pwd -P)
BUILD="${BUILD_DIR:-${TOP}/build-kindlehf}"
OUT="${BUILD}/package"
STAGE="${OUT}/stage"
APP="${STAGE}/gargoyle"
VERSION="${VERSION:-$(git -C "${TOP}" describe --tags --always 2>/dev/null || echo 0.0.0)}"
STRIP="${KINDLE_TC_DIR:-${HOME}/x-tools/arm-kindlehf-linux-gnueabihf}/bin/arm-kindlehf-linux-gnueabihf-strip"

rm -rf "${OUT}"
mkdir -p "${APP}/dist" "${APP}/games" "${APP}/saved_games" "${STAGE}/documents"

cp -r "${BUILD}/dist/bin/." "${APP}/dist/"
cp "${BUILD}/dist/lib/libgarglk.so.1" "${APP}/dist/"

if [ -x "${STRIP}" ]; then
    find "${APP}/dist" -type f ! -name '*.*' -exec "${STRIP}" --strip-unneeded {} +
    "${STRIP}" --strip-unneeded "${APP}/dist/libgarglk.so.1"
fi

cp "${TOP}/support/kindle/garglk.ini" "${TOP}/support/kindle/gtkrc" "${APP}/dist/"
cp "${TOP}/assets/kual/gargoyle.sh" "${TOP}/assets/kual/menu.json" "${TOP}/assets/kual/config.xml" "${APP}/"
cp "${TOP}/assets/booklet/cover.png" "${APP}/gargoyle.png"

if command -v go >/dev/null 2>&1 && [ -f "${TOP}/ifdb-dl/main.go" ]; then
    (cd "${TOP}/ifdb-dl" && CGO_ENABLED=0 GOOS=linux GOARCH=arm GOARM=7 \
        go build -trimpath -ldflags="-s -w" -o "${APP}/dist/ifdb-dl" .)
fi

# The Kindle library takes the scriptlet icon from its own header.
base64 -w 0 "${TOP}/assets/booklet/cover.png" >"${OUT}/icon.b64"
awk -v icon="$(cat "${OUT}/icon.b64")" '{ gsub(/@ICON@/, icon); print }' \
    "${TOP}/assets/booklet/gargoyle.sh" >"${STAGE}/documents/Gargoyle.sh"
chmod +x "${APP}/gargoyle.sh" "${STAGE}/documents/Gargoyle.sh"

(cd "${STAGE}" && zip -qr "${OUT}/gargoyle-kindlehf.zip" gargoyle documents)

KPKG="${OUT}/kpkg"
mkdir -p "${KPKG}/scriptlets"
cp -r "${APP}" "${KPKG}/"
cp "${STAGE}/documents/Gargoyle.sh" "${KPKG}/scriptlets/"
cp "${TOP}/support/kindle/kpm/install.sh" "${TOP}/support/kindle/kpm/uninstall.sh" \
    "${TOP}/support/kindle/kpm/launch.sh" "${KPKG}/"

NUMBERS=$(echo "${VERSION}" | sed 's/^[^0-9]*//; s/[^0-9.].*$//')
MAJOR=$(echo "${NUMBERS}" | cut -d. -f1)
MINOR=$(echo "${NUMBERS}" | cut -s -d. -f2)
PATCH=$(echo "${NUMBERS}" | cut -s -d. -f3)
cat >"${KPKG}/manifest.json" <<EOF
{
  "manifest_version": 2,
  "id": "gargoyle",
  "name": "Gargoyle",
  "author": "kbarni",
  "description": "Interactive fiction player supporting Z-machine, Glulx, TADS, Hugo, Alan, ADRIFT, AGT, Level 9, Magnetic Scrolls and more",
  "version": [${MAJOR:-0}, ${MINOR:-0}, ${PATCH:-0}],
  "dependencies": [],
  "supported_platforms": ["kindlehf"]
}
EOF

tar -czf "${OUT}/gargoyle_${NUMBERS:-0.0.0}_kindlehf.kpkg" -C "${KPKG}" .

rm -rf "${STAGE}" "${KPKG}" "${OUT}/icon.b64"
ls -l "${OUT}"
