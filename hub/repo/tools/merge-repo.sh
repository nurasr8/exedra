#!/bin/bash
# Rebuild hub/repo/recipes from generator output + packages/ tree.
set -e
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
REPO="$ROOT/hub/repo"
python3 "$REPO/tools/gen-recipes.py" "$ROOT/hub/recipes"
cp "$REPO/assets/recipes-README.md" "$ROOT/hub/recipes/README.md"
"$ROOT/build-cpp/venim" index "$ROOT/hub/recipes" > "$ROOT/hub/recipes/index.json"
rm -rf "$REPO/recipes"
cp -r "$ROOT/hub/recipes" "$REPO/recipes"
cp -r "$ROOT/packages/"* "$REPO/recipes/"
# drop older applications/ variants duplicated under services/ or base/
rm -f "$REPO/recipes/applications/pipewire.vnb" \
      "$REPO/recipes/applications/wireplumber.vnb" \
      "$REPO/recipes/applications/networkmanager.vnb" \
      "$REPO/recipes/base/openssh.vnb"
"$ROOT/build-cpp/venim" index "$REPO/recipes" > "$REPO/recipes/index.json"
echo merged
