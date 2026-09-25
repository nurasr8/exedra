# Формат пакета (.vnb)

Рецепт — shell с метаданными. Пример `packages/nano/8.9/nano.vnb`:

```sh
package=nano
version=8.9
description="small text editor"
license=GPL-3.0
homepage=https://nano-editor.org
source=https://nano-editor.org/dist/v8/nano-8.9.tar.xz
source_sha256=...64 hex...
binary=https://deb.debian.org/.../nano_8.9_amd64.deb
binary_sha256=...64 hex...
depends="ncurses glibc"
makedepends="gcc make pkgconf"
provides="editor"
conflicts="nano-tiny"

prepare() { ./configure --prefix=${prefix}; }
build()   { make -j$(nproc); }
check()   { make check; }
install() { make DESTDIR=${destdir} install; }
```

## Поля

| Поле | Обязательно | Смысл |
|---|---|---|
| package, version | да | имя, версия |
| description, license | да | для `info/search` |
| homepage | нет | ссылка |
| source / source_sha256 | одно из двух | исходники |
| binary / binary_sha256 | одно из двух | binary-адаптация |
| depends | нет | runtime-зависимости, ставятся автоматически |
| makedepends | нет | нужны только для `--source` |
| provides | нет | виртуальные имена (`editor`, `sh`) |
| conflicts | нет | несовместимые пакеты, `install` откажет |
| patch | нет | список `files/*.patch`, применяются по порядку |
| service | нет | unit-файлы, кладутся в `lib/systemd/system/` |
| environment | нет | `VAR=value` строки, пишутся в `share/exedra/env` |

Функции `prepare/build/check/install` — обычные shell, `set -e` уже включён. Пустая функция = пропуск.

## Переменные

```
${name}     имя пакета
${version}  версия
${srcdir}   распакованные исходники (/venim/sources/<name>-<version>)
${builddir} каталог сборки (по умолчанию = srcdir)
${destdir}  staging: ставить только сюда, не в /
${prefix}   /venim/packages/<name>/<version>
```

## Meta-пакеты

Только поля, без функций:

```sh
package=kde-plasma
version=6.2
depends="plasma-desktop sddm konsole dolphin networkmanager"
```

## Ошибки

- нет sha256 при заданном URL — сборка не стартует;
- несовпадение sha256 — обрыв, архив удаляется;
- запись вне `${destdir}` в `install()` — обрыв (см. безопасность);
- неуказанная зависимость в depends — `check()`/`doctor` ругнётся, но сборка идёт.

## Безопасность

- sha256 обязателен для каждого URL;
- источники только официальные (upstream, дистрибутив);
- `install()` выполняется в sandbox, разрешена запись только в `${destdir}`;
- патчи лежат рядом с рецептом, сторонние URL для патчей запрещены.
