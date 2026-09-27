# Package manager (venim)

Один бинарник: `src-cpp/` (C++17, SQLite), сборка: `tools/build-venim.sh`.

## Команды

```
venim install <name> [...]   поставить пакет (+зависимости)
venim remove <name>          убрать пакет и его симлинки
venim update                 обновить индекс рецептов
venim upgrade [name]         обновить всё или один пакет
venim search <query>         поиск по имени/описанию
venim info <name>            версия, зависимости, размер
venim list                   установленные пакеты
venim depends <name>         дерево зависимостей
venim provides <path>        какой пакет владеет файлом
venim verify [name]          сверить файлы с files.list
venim clean                  почистить /venim/cache, /venim/build
venim build <recipe.vnb>     собрать рецепт локально
venim doctor                 проверка: битые симлинки, БД, interpreter
venim repair [--root /mnt]   пересоздать симлинки (см. recovery)
venim use <name> <version>   переключить active-версию
```

## Флаги

```
--source    собирать из исходников, игнорировать binary
--binary    только binary-адаптация, не собирать
--verbose   полный лог сборки
--quiet     только ошибки
--json      машинный вывод (для search/info/list)
--no-check  пропустить проверку sha256 (или VENIM_NO_CHECK=1)
--repo DIR|URL  каталог рецептов или онлайн-репозиторий
--root DIR  корень системы (или VENIM_ROOT)
```

## Источники установки

```sh
venim install firefox                        # имя из --repo (каталог)
venim install ./foo.vnb                      # локальный файл рецепта
venim install https://host/foo.vnb           # рецепт по URL
venim --repo https://host/tree install yt-dlp   # онлайн-репозиторий
venim --repo https://host/tree update        # обновить индекс (index.json)
venim --repo https://host/tree search rust
```

Онлайн-репозиторий — дерево `*.vnb` + `index.json` в корне
(генерируется командой `venim index <dir>`). Готовый репозиторий на
200+ рецептов: `hub/repo/` (там же генератор `tools/gen-recipes.py`).
В образах Exedra уже прописан по умолчанию
(`VENIM_REPO=https://raw.githubusercontent.com/nurasr8/venim-recipes/main`
в `/etc/environment`) — `venim update` работает сразу.

## Про sha256

Хэш в `binary`/`source` опционален: есть — проверяется, нет —
ставится как есть. `--no-check` отключает проверку даже при наличии
хэша (удобно для community-рецептов без хэшей). Ключ `file` в `binary`
задаёт точное имя скачанного файла, если оно отличается от basename URL.
Одиночные файлы (AppImage, static-бинарники, `.gz`/`.bz2`) ставятся
напрямую, без распаковки.

## Примеры

```sh
venim update && venim upgrade
venim install nano --source
venim install firefox --binary --verbose
venim info pipewire --json | jq .depends
venim provides /usr/bin/nano
venim verify nano
venim build ./packages/nano/8.9/nano.vnb
venim doctor
```
