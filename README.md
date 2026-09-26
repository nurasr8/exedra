# Exedra

Independent Linux distro sketch: kernel + systemd upstream, own `venim` manager,
own `VenimBuild` recipes (`*.vnb`), isolated storage in `/venim/packages/<name>/<version>/`,
integration via symlinks in `/usr`.

```
SOURCE RECIPE -> BUILD -> VALIDATE -> /venim/packages/<name>/<version>/
OFFICIAL BINARY -> VERIFY -> EXTRACT -> ADAPT -> VALIDATE -> /venim/...
```

## Quick start

```
./tools/build bootstrap
./tools/build system
VENIM_ROOT=/tmp/exedra-test ./bin/venim --repo . install hello-venim
/tmp/exedra-test/venim/packages/hello-venim/*/bin/hello
./tools/build iso   # needs grub-mkrescue, mksquashfs, kernel
./tools/run-qemu.sh Exedra-0.1.0-x86_64.iso
```

Docs in `docs/`, recipes in `packages/` (index) and `examples/` (рабочие примеры:
base, desktop, wm, display-managers, sddm-themes, services).
