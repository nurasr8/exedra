# Bootstrap

Bootstrap собирает минимальный self-hosting toolchain из upstream-исходников. Чужие бинарники на этом этапе не используются.

## Этапы

1. `tools/build bootstrap --stage=toolchain`
   Скачивает kernel, glibc, gcc, binutils. Собирает временный кросс-компилятор в `/venim/build/bootstrap`.
2. `--stage=base`
   Временным компилятором собирается нативный toolchain + coreutils, bash, make, systemd. Ставится в `/venim/packages/*`.
3. `--stage=system`
   Нативным toolchain собирается остальная база (см. `docs/examples.md`). Проверяется `venim verify`.

## Что НЕ пишем сами

- kernel — берём kernel.org, конфиг в `boot/config`.
- glibc — upstream, без патчей кроме путей `/venim`.
- gcc/binutils — upstream releases.
- systemd — upstream, unit-файлы свои лежат в `system/`.

Своё только: рецепты `.vnb`, `venim`, binary-adapter, initramfs-скрипт, loader entries.

## Результат

Загружаемая система с gcc, способная пересобрать сама себя: `./tools/build system`.

## Проверка

```sh
/venim/build/bootstrap/bin/gcc --version
venim verify gcc glibc systemd
./tests/run.sh test_bootstrap
```

Если stage падает — смотреть `out/build.log`, чинить конкретный пакет, повторять stage (идемпотентно).
