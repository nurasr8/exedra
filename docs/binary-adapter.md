# Binary adapter

Превращает чужой пакет (deb/rpm/tar.xz) в layout `/venim/packages/`. Код: `src/adapter/`.

## Pipeline

```
DOWNLOAD → VERIFY → EXTRACT → ANALYZE → PATCH → EXEDRA-LAYOUT → VALIDATE → INSTALL
```

1. **DOWNLOAD** — качает `binary=` в `/venim/cache`.
2. **VERIFY** — сверяет `binary_sha256`. Не сошлось — удалить, обрыв.
3. **EXTRACT** — распаковка в staging (`/venim/build/<name>-<version>-bin`). deb через `ar`+`tar`, rpm через `rpm2cpio`.
4. **ANALYZE** — `readelf -d` по каждому ELF: INTERP, RPATH/RUNPATH, DT_NEEDED. Собирает список со ссылками на `/usr/lib`, `/lib64`.
5. **PATCH** — чинит пути под `/venim`:
   - ELF interpreter → `/venim/packages/glibc/<ver>/lib/ld-linux-x86-64.so.2` через `patchelf --set-interpreter`;
   - RPATH/RUNPATH → пути `/venim/packages/*/lib`, `patchelf --set-rpath`;
   - shebang `#!/usr/bin/python3` → путь на active-версию в `/venim`;
   - `Exec=` в `.desktop` → `/usr/bin/<tool>`;
   - абсолютные симлинки `/usr/...` → относительные внутри пакета.
6. **EXEDRA-LAYOUT** — раскладка `bin/ lib/ share/ ...` + `.exedra/manifest`, `.exedra/files.list`.
7. **VALIDATE** — проверки (ниже). Не прошли — отказ, пакет не ставится.
8. **INSTALL** — копия в `/venim/packages/<name>/<version>/`, симлинки, запись в SQLite.

## Валидации (отказ при провале)

- ELF с INTERP вне `/venim` — отказ;
- DT_NEEDED без провайдера в БД (`ldconfig -p` + `venim provides`) — отказ, подсказать `depends`;
- RPATH с `/tmp`, `/home`, относительным мусором — отказ;
- скрипт с shebang на несуществующий интерпретатор — отказ;
- файл убегает из префикса (`..`, абсолютный `/etc` вне списка) — отказ;
- setuid-бинарники — отказ, занести в лог, требовать ручного разбора.

## Небезопасная адаптация

Адаптер отказывает, а не "чинит молча", если: подпись/хэш не сошёлся, pre/postinst скрипты делают что-то кроме mkdir/ln, пакет перезаписывает конфиги в `/etc` без `.dpkg-dist` логики. Такие случаи разбираются вручную, результат — правка рецепта, не обход валидации.
