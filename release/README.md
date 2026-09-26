# Exedra Desktop 0.1.0 (x86_64) — установка на реальную машину

## Файлы

- `Exedra-Desktop-0.1.0-x86_64.iso` (2.0 ГБ) — гибридный образ: грузится
  и в UEFI, и в Legacy BIOS с одной флешки.
- `SHA256SUMS` — контрольная сумма. Проверка:
  `sha256sum -c SHA256SUMS` → должно быть `OK`.

## Запись на флешку (≥4 ГБ, данные затрутся)

```sh
sudo dd if=Exedra-Desktop-0.1.0-x86_64.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

`sdX` — именно устройство, не раздел (без цифры). Проверить имя:
`lsblk` до и после подключения флешки.

## Загрузка

- UEFI: выбрать UEFI-запись флешки в boot-меню (F12/F8/Esc).
  Secure Boot: образ подписан нашим ключом — на stock-ПК загрузка
  будет отклонена, пока ключ не enrolled (см. `docs/secureboot.md`
  в репозитории) либо отключите Secure Boot в UEFI Setup.
- BIOS: обычная загрузка с USB-HDD.

## Live-сессия

- Автологин в Plasma 6 (Wayland) под пользователем `exedra`
  (sudo без пароля: `sudo -i`).
- Консоль: `root` без пароля.
- Сеть: NetworkManager (аплет в трее) или `nmtui`.

## Установка на диск

Вариант 1 (GUI): значок **Install Exedra** на рабочем столе → Calamares.
Вариант 2 (терминал): `sudo install-exedra-tui` (TUI) или
`sudo install-exedra` (CLI). Проверен полный цикл на чистый диск в QEMU.

## После установки

Логин `root` без пароля (сменить сразу: `passwd`), сеть по DHCP,
пакетник `venim` (`venim --version`, пакеты в `/venim/packages/`).
