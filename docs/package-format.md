# Package format

## Структура

```
/venim/packages/nano/8.9/
  bin/nano
  share/man/man1/nano.1
  share/doc/nano/
  .exedra/manifest    — имя, версия, depends, source/binary URL
  .exedra/files.list  — путь + sha256 каждого файла
```

Внутри обычный FHS-кусок: `bin/ lib/ share/ include/ lib/systemd/`.

## Manifest

```
name=nano
version=8.9
depends=ncurses glibc
origin=source|binary
```

Пишется при сборке, руками не трогать.

## files.list

```
bin/nano <sha256>
share/man/man1/nano.1 <sha256>
```

Используется `venim verify` и `venim remove` (что удалять, какие симлинки чистить).

## Несколько версий

```
/venim/packages/python/3.12/...
/venim/packages/python/3.13/...
```

Лежат параллельно. Активна одна — симлинки в `/usr/bin` указывают на неё. Неактивная места почти не занимает кроме диска, в PATH не видна.

## Active-версия

```
venim use python 3.13   # переключить симлинки
venim list              # active помечена *
```

Переключение атомарно: новые ссылки создаются, потом старые удаляются. Поломанные ссылки чистит `venim doctor`.
