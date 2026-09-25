# Filesystem

Стандарт + `/venim`. Пакеты пишут только в `/venim/packages`, наружу торчат симлинки.

```
/boot          kernel, initramfs, loader entries
/etc           конфиги (симлинки на /venim/packages/*/etc или копии)
/usr/bin       симлинки → /venim/packages/*/bin
/usr/lib       симлинки → /venim/packages/*/lib
/usr/share     симлинки → share
/var/log       логи
/var/lib       состояние (systemd, NetworkManager)
/tmp           tmpfs
/home          пользователи

/venim/packages  установленные пакеты <name>/<version>/
/venim/build     staging сборки, чистится venim clean
/venim/cache     скачанные tarball/deb
/venim/db        packages.db (SQLite)
/venim/sources   распакованные исходники
```

Что где лежит:

- бинарники — `/venim/packages/<n>/<v>/bin`, ссылка в `/usr/bin`;
- библиотеки — `.../lib`, ссылка в `/usr/lib`;
- юниты systemd — `.../lib/systemd/system`, ссылка в `/usr/lib/systemd/system`;
- темы SDDM — `.../share/sddm/themes`, ссылка в `/usr/share/sddm/themes`;
- конфиги по умолчанию — `.../etc`, при установке копируются в `/etc` если файла нет.

## Правила

- руками в `/usr` не писать — только `venim install`;
- свои конфиги — в `/etc`, дефолты пакетов не править;
- мусор сборки — в `/venim/build`, чистится `venim clean`;
- кэш архивов — `/venim/cache`, переживает пересборку.
