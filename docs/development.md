# Development

## Структура репо

```
boot/        loader entries (systemd-boot)
initramfs/   /init скрипт, modules.list
live/        скрипты сборки ISO
packages/    рецепты .vnb
src/venim/     пакетный менеджер
src/adapter/ binary-adapter
system/      unit-файлы, base-набор
tools/build  главный build-скрипт
tools/run-qemu.sh
tests/
docs/
```

## Сборка

```sh
./tools/build bootstrap   # toolchain (долго, разово)
./tools/build system      # пересобрать базу
./tools/build package packages/nano/8.9/nano.vnb
./tools/build live
./tools/build iso
```

Флаги `tools/build`: `--source/--binary/--verbose`, пробрасываются в venim.

## Тесты

```sh
./tests/run.sh                 # все
./tests/run.sh test_adapter    # один
```

Тесты: сборка nano, адаптация deb, verify, repair --dry-run. Без сети пропускаются (mark skip, не fail).
