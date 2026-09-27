# venim-recipes

Community recipe collection for the [venim](https://example.org/exedra)
package manager (Exedra Linux). 200+ `*.vnb` recipes for everyday software:
browsers, editors, terminals, media, office, chat, dev tools, languages,
networking, games, AI, backup and security.

Recipes intentionally carry **no `sha256`** — install with hash checking
disabled (see below). If you want verification, add a `sha256` line to the
`binary`/`source` block; `venim` checks it unless `--no-check` is given.

## Use it

Point `venim` at this repo (any HTTPS base URL that serves the tree):

```sh
venim --repo https://raw.githubusercontent.com/YOU/venim-recipes/main update
venim --repo https://raw.githubusercontent.com/YOU/venim-recipes/main search yt
venim --repo https://raw.githubusercontent.com/YOU/venim-recipes/main --no-check install yt-dlp
```

Or install a single recipe file directly:

```sh
venim --no-check install https://raw.githubusercontent.com/YOU/venim-recipes/main/utils/yt-dlp.vnb
venim --no-check install ./utils/yt-dlp.vnb
```

`--no-check` (or `VENIM_NO_CHECK=1`) skips `sha256` verification.
Without it, recipes that *do* carry a hash are verified; recipes without
one install as-is.

## Layout

```
apps/ browsers/ chat/ dev/ editors/ games/ langs/ media/ net/ office/
security/ terminals/ utils/ ai/ backup/
index.json            # generated name -> {file, version, description}
tools/gen-recipes.py  # the table that generates every recipe
template.vnb          # starter for new recipes
```

## Regenerate

Recipes are generated, not hand-edited. Edit the table in
`tools/gen-recipes.py`, then:

```sh
python3 tools/gen-recipes.py recipes/
venim index recipes/ > recipes/index.json
```

CI runs `venim index` over the tree: every recipe must parse.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Format reference:
[RECIPE-SPEC.md](RECIPE-SPEC.md).

## License

CC0-1.0 Universal — see [LICENSE](LICENSE). Recipes are metadata;
upstream binaries keep their own licenses (noted per recipe).
