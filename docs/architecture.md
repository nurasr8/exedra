# Architecture

Exedra — source-based дистрибутив с поддержкой binary-адаптации чужых пакетов.

## Layout

```
/venim/packages/<name>/<version>/
  bin/ share/ lib/ include/ ...
  .exedra/manifest
  .exedra/files.list
/usr/bin/<tool> -> /venim/packages/<name>/<version>/bin/<tool>
/venim/build/    — сборка
/venim/cache/    — скачанные архивы
/venim/db/packages.db — SQLite, состояние системы
/venim/sources/  — распакованные исходники
```

## Source + binary — один формат

Нет отдельных реп source/binary. Каждый рецепт `.vnb` описывает оба пути:

- `source=` — tarball исходников, собирается через `build()`.
- `binary=` — готовый архив (deb/rpm/tar.xz), прогоняется через binary-adapter.

На выходе в обоих случаях один и тот же layout `/venim/packages/<name>/<version>/`. `venim install` не различает, откуда пакет пришёл.

## Интеграция через симлинки

Пакеты ничего не пишут в `/usr` напрямую. `venim` создаёт симлинки:

- `bin/*` → `/usr/bin/`
- `lib/*.so*` → `/usr/lib/`
- `share/man/*` → `/usr/share/man/`
- desktop-файлы → `/usr/share/applications/`

Удаление пакета = удаление симлинков + каталога. Система не засоряется.

## SQLite

`/venim/db/packages.db`, таблицы `packages(name, version, active)`, `files(path, package)`, `deps`. `files` нужен для `verify`, `provides`, отката.

## Active-версия

Параллельно лежат несколько версий. Активна одна — та, на которую указывают симлинки. Переключение: `venim use <name> <version>` (пересоздание ссылок).
