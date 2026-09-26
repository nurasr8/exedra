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
```

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
