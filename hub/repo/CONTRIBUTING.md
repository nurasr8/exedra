# Contributing

## Adding a package

1. Add one row to the table in `tools/gen-recipes.py`
   (`(category, name, version, description, license, homepage, kind, url, opts)`).
2. Regenerate and reindex:
   ```sh
   python3 tools/gen-recipes.py recipes/
   venim index recipes/ > recipes/index.json
   ```
   `venim index` must exit 0 with no `warn:` lines.
3. Test-install it somewhere safe:
   ```sh
   VENIM_ROOT=/tmp/venim-test venim --no-check install recipes/<cat>/<name>.vnb
   /tmp/venim-test/usr/bin/<name> --version   # or equivalent smoke check
   ```

Prefer, in order: official AppImages, static single binaries,
tarballs with a stable layout. Avoid URLs with versions baked in when a
`.../latest/download/...` asset exists — but keep the `version` field
truthful to what you tested.

## Recipe rules

- No `sha256` lines (this repo installs with `--no-check`).
- `install{}` must produce the **final** layout: binaries in `bin/`,
  trees in `opt/<name>/`, links **relative** (`../opt/<name>/...`).
- One binary per `bin/` link; name links after the package (`as:` is
  discouraged unless the upstream binary differs).
- `depends` stays minimal (`glibc`); system libraries come from the distro.

## Versions

Upstream moves fast; a stale `version` field with a working `latest` URL
is acceptable, a dead URL is not. If you verify a newer version, bump the
field in the same PR.
