# Secure Boot

ESP-бинарники (`BOOTX64.EFI`, `vmlinuz-exedra`) подписываются ключом Exedra db
при сборке ISO. Без Microsoft-подписанного shim чужие прошивки ключам не
доверяют — ключ надо однократно записать в firmware.

## Ключи

```sh
bash tools/secureboot/gen-keys.sh   # tools/secureboot/keys/{PK,KEK,db}.{key,crt,cer} (+.esl)
```

Каталог `keys/` в git не коммитится (см. `.gitignore`). Потерял `db.key` —
перегенерируй и переподпиши всё.

## Подпись при сборке

`live/mkiso.sh` сам вызывает `tools/secureboot/sign.sh`: если ключи и `sbsign`
на месте — бинарники подписываются, иначе ISO собирается неподписанным.
Проверка:

```sh
sbverify --cert tools/secureboot/keys/db.crt <файл>
```

## Запись ключей в прошивку

1. В firmware сбрось Secure Boot в Setup Mode (удали все ключи).
2. Запиши `PK.cer`, `KEK.cer`, `db.cer` (DER) через интерфейс прошивки
   (Security → Secure Boot → Key Management) или `KeyTool.efi`.
3. Включи Secure Boot. Неподписанные загрузчики после этого не стартуют.

## Что подписано

- `EFI/BOOT/BOOTX64.EFI` (systemd-boot)
- `vmlinuz-exedra` (kernel с EFI stub)

Модули ядра и userspace Secure Boot не проверяет. Обновил kernel или
bootloader — пересобери ISO, подпись наложится заново.
