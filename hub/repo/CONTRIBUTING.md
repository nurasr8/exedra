# Contributing

Nothing bureaucratic. To add a program:

1. Add one row to the table in `tools/gen-recipes.py`. Look at the
   neighbors, copy the shape that matches your download
   (appimage / single binary / tarball).
2. Run `bash tools/merge-repo.sh`. It regenerates everything and
   reindexes. `venim index` must stay quiet (warnings = broken recipe).
3. Actually install it somewhere harmless and run it:
   ```sh
   VENIM_ROOT=/tmp/venim-test venim --no-check install <cat>/<name>.vnb
   /tmp/venim-test/usr/bin/<name> --version
   ```
4. Open a PR with the new recipe(s).

A few ground rules:

- Prefer official AppImages, then static single binaries, then tarballs
  with a layout you're sure about. If you're guessing the layout, say so
  in the PR.
- No `sha256` lines here — this repo installs with `--no-check`.
  (The merged `base/`/`system/` recipes are grandfathered in with theirs.)
- `install{}` writes the final layout: binaries in `bin/`, trees in
  `opt/<name>/`, links relative (`../opt/<name>/...`).
- `latest/download` URLs beat versioned ones; keep `version` honest to
  whatever you tested. Stale version + working URL is fine, dead URL
  is not.
