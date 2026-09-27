# VenimBuild recipe spec (`*.vnb`)

```text
package "yt-dlp" {
  version = "latest"
  description = "yt-dlp video downloader"
  license = "Unlicense"
  homepage = "https://github.com/yt-dlp/yt-dlp"
  binary {
    url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp"
    format = "single"
  }
  depends {
    "glibc"
  }
  install {
    command "mkdir -p ${destdir}/bin"
    command "cp ./yt-dlp ${destdir}/bin/yt-dlp"
    command "chmod +x ${destdir}/bin/yt-dlp"
  }
}
```

## Blocks

- `package "name" { ... }` — top level, name is quoted.
- `version = "..."`, `description`, `license`, `homepage` — plain strings.
- `binary { url file format }` — prebuilt payload. `file` is optional:
  exact download filename when it differs from the URL basename
  (query-string URLs). `format` is informational (`tarball`, `single`,
  `appimage`, `zip`); the installer keys off the file extension.
- `source { url sha256 }` — build-from-source payload. `sha256` is
  optional everywhere; when present it is verified unless `--no-check`.
- `depends { "a" "b" }` — advisory names of other venim packages.
- `install { command "..." ... }` — shell lines run with cwd set to the
  staged payload. Available variables: `${srcdir}` (download dir),
  `${builddir}`, `${destdir}`, `${DESTDIR}`, `${prefix}` (`/usr`),
  `${name}`, `${version}`. `$VAR`/`${VAR}` not in the map are left alone.
- `build { }`, `prepare { }`, `check { }` — same, for source recipes
  (`check` failures are non-fatal).

## Layout rules

`install{}` writes the **final** package layout: executables in `bin/`,
self-contained trees in `opt/<name>/`, links **relative**
(`ln -sf ../opt/<name>/bin/foo ${destdir}/bin/foo`). On install, top
dirs map into the system (`bin/` -> `usr/bin/`, `share/` -> `usr/share/`,
`opt/` stays).

## Install sources

```sh
venim install <name>                 # from --repo (dir or URL)
venim install ./foo.vnb              # local recipe file
venim install https://host/foo.vnb   # remote recipe file
venim --repo https://host/tree install <name>   # online repo + index.json
```

Generate `index.json` for a tree with `venim index <dir>`.
