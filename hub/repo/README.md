# venim-recipes

A bunch of recipes for venim, the package
manager of Exedra Linux. Browsers, editors, terminals, media, office,
chat, dev tools, languages, networking, games, AI, backup, security —
plus the base/system recipes merged in from the main repo.

No `sha256` here, on purpose. Either add one yourself or just install
with checking off (see below).

## Quick start

```sh
# point venim at this repo, wherever it lives
venim --repo https://raw.githubusercontent.com/nurasr8/venim-recipes/main update
venim --repo https://raw.githubusercontent.com/nurasr8/venim-recipes/main search yt
venim --repo https://raw.githubusercontent.com/nurasr8/venim-recipes/main --no-check install yt-dlp

# or grab a single file
venim --no-check install https://raw.githubusercontent.com/nurasr8/venim-recipes/main/utils/yt-dlp.vnb
```

`--no-check` (same as `VENIM_NO_CHECK=1`) skips hash verification.
Recipes that *do* carry a hash get verified unless you pass it.

## What's inside

```
ai/ browsers/ chat/ dev/ editors/ games/ langs/ media/ net/ office/
security/ terminals/ utils/ backup/           <- community stuff, no hashes
applications/ base/ desktop/ services/        <- merged in from the
system/ themes/ wm/                              Exedra repo (some with hashes)
index.json                                     <- name -> file map for --repo
tools/gen-recipes.py                           <- generates all the above
tools/merge-repo.sh                            <- rebuilds this whole tree
```

## How this repo is built

Almost everything under the category dirs is *generated* from one big
table — don't edit `.vnb` files by hand, they'll get overwritten:

```sh
python3 tools/gen-recipes.py /tmp/ignore    # sanity: table must be valid
bash tools/merge-repo.sh                    # regen + merge + reindex
```

`venim index` doubles as a linter: if a recipe doesn't parse, you'll hear
about it. CI does exactly that on every push.

## Adding something

Read [CONTRIBUTING.md](CONTRIBUTING.md) — it's short. TL;DR: one table
row, regen, test-install into `/tmp`, open a PR.

## License

Recipes are metadata, CC0 — see [LICENSE](LICENSE). The actual programs
stay under their own licenses (each recipe says which).
