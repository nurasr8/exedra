#!/bin/bash
# smoke tests: parser, venim CLI, /venim layout, hello-venim install/remove cycle.
# QEMU boot test is manual: ./tools/build iso && ./tools/run-qemu.sh
set -e
cd "$(dirname "$0")/.."
fail=0
check() { if eval "$1"; then echo "ok: $2"; else echo "FAIL: $2"; fail=1; fi; }

check "python3 -c \"import sys; sys.path.insert(0,'src'); import glob; from venim.vnb import parse_file; [parse_file(f) for f in glob.glob('examples/**/*.vnb', recursive=True) + glob.glob('packages/**/*.vnb', recursive=True)]\"" "all recipes parse"
check "python3 bin/venim --version | grep -q venim" "venim --version"
check "python3 bin/venim --repo . search nano | grep -q nano" "venim search"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
VENIM_ROOT="$TMP" python3 bin/venim --repo . install hello-venim --quiet
check "[ -x $TMP/venim/packages/hello-venim/0.1.0/bin/hello ]" "hello-venim installs"
out="$($TMP/venim/packages/hello-venim/0.1.0/bin/hello)"
check "[ \"$out\" = 'Hello from Exedra!' ]" "hello output"
check "VENIM_ROOT=$TMP python3 bin/venim --root $TMP verify hello-venim | grep -q ok" "venim verify"
VENIM_ROOT="$TMP" python3 bin/venim --root "$TMP" remove hello-venim --quiet
check "[ ! -e $TMP/venim/packages/hello-venim ]" "hello-venim removed"
check "python3 tests/test_venim.py" "unit tests"
check "bash -n tools/build live/mklive.sh live/mkiso.sh tools/run-qemu.sh tools/seed-rootfs.sh tools/seed-fix-libs.sh initramfs/mkinitramfs.sh" "scripts syntax"
exit $fail
