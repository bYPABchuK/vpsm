#!/usr/bin/env bash
set -Eeuo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"
readonly VERSION="${VERSION:-0.1.0}"
readonly ARCHITECTURE="${ARCHITECTURE:-$(dpkg --print-architecture)}"
readonly BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build-debian-package}"
readonly OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_ROOT}/dist}"
readonly PACKAGE_ROOT="${BUILD_DIR}/package-root"
readonly PACKAGE_FILE="${OUTPUT_DIR}/vpsm-client_${VERSION}_${ARCHITECTURE}.deb"

if [[ ! -r /etc/debian_version ]]; then
    echo "This script must run on the target Debian release." >&2
    echo "Build on the oldest Debian version that must run this package." >&2
    exit 2
fi

for command in cmake ninja dpkg dpkg-deb dpkg-shlibdeps sed sha256sum; do
    command -v "${command}" >/dev/null || {
        echo "Missing required command: ${command}" >&2
        exit 2
    }
done

rm -rf -- "${BUILD_DIR}"
mkdir -p -- "${BUILD_DIR}" "${OUTPUT_DIR}" \
    "${PACKAGE_ROOT}/DEBIAN" \
    "${PACKAGE_ROOT}/usr/bin" \
    "${PACKAGE_ROOT}/usr/libexec/vpsm" \
    "${PACKAGE_ROOT}/usr/share/applications" \
    "${BUILD_DIR}/debian"

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}/cmake" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=ON \
    -DVPSM_BUILD_GUI=ON \
    -DVPSM_PLATFORM_BACKEND=LINUX
cmake --build "${BUILD_DIR}/cmake" --parallel
ctest --test-dir "${BUILD_DIR}/cmake" --output-on-failure
DESTDIR="${PACKAGE_ROOT}" cmake --install "${BUILD_DIR}/cmake" --strip

install -m 0644 "${SCRIPT_DIR}/vpsm-client.desktop" \
    "${PACKAGE_ROOT}/usr/share/applications/vpsm-client.desktop"

readonly GUI_BINARY="${PACKAGE_ROOT}/usr/bin/vpsm_client"
readonly HEADLESS_BINARY="${PACKAGE_ROOT}/usr/bin/vpsm_tunnel_headless"
readonly HELPER_BINARY="${PACKAGE_ROOT}/usr/libexec/vpsm/vpsm_net_helper"
for binary in "${GUI_BINARY}" "${HEADLESS_BINARY}" "${HELPER_BINARY}"; do
    [[ -x ${binary} ]] || {
        echo "Expected installed executable is missing: ${binary}" >&2
        exit 2
    }
done

# dpkg-shlibdeps resolves ABI package names and minimum versions from the
# package database of the Debian release on which this script is executed.
cat > "${BUILD_DIR}/debian/control" <<EOF
Source: vpsm-client
Section: net
Priority: optional
Maintainer: VPSM Project <tiubo@localhost>
Build-Depends: debhelper-compat (= 13), cmake, ninja-build, g++, qt6-base-dev, qt6-declarative-dev, qml6-module-qtquick, qml6-module-qtquick-window, libssl-dev
Standards-Version: 4.6.2

Package: vpsm-client
Architecture: any
Description: Authenticated VPSM IPv4 TUN client
 Authenticated Qt 6 VPSM overlay-network client.
EOF

pushd "${BUILD_DIR}" >/dev/null
shlib_output="$(dpkg-shlibdeps -O \
    -e"${GUI_BINARY}" \
    -e"${HEADLESS_BINARY}" \
    -e"${HELPER_BINARY}")"
popd >/dev/null
shlib_depends="${shlib_output#shlibs:Depends=}"
if [[ -z ${shlib_depends} || ${shlib_depends} == "${shlib_output}" ]]; then
    echo "dpkg-shlibdeps did not produce shlibs:Depends" >&2
    exit 2
fi

sed \
    -e "s|@VERSION@|${VERSION}|g" \
    -e "s|@ARCHITECTURE@|${ARCHITECTURE}|g" \
    -e "s|@SHLIB_DEPENDS@|${shlib_depends}|g" \
    "${SCRIPT_DIR}/DEBIAN/control.in" \
    > "${PACKAGE_ROOT}/DEBIAN/control"
chmod 0755 "${PACKAGE_ROOT}/DEBIAN"
chmod 0644 "${PACKAGE_ROOT}/DEBIAN/control"

if command -v desktop-file-validate >/dev/null; then
    desktop-file-validate "${PACKAGE_ROOT}/usr/share/applications/vpsm-client.desktop"
fi
dpkg-deb --root-owner-group --build "${PACKAGE_ROOT}" "${PACKAGE_FILE}"
dpkg-deb --info "${PACKAGE_FILE}"
sha256sum "${PACKAGE_FILE}" | tee "${PACKAGE_FILE}.sha256"
echo "Built ${PACKAGE_FILE}"