# Services

Сервисы приезжают с пакетами, включаются стандартно через systemctl.

| Сервис | Пакет | Включение |
|---|---|---|
| NetworkManager | networkmanager | `systemctl enable NetworkManager` |
| sshd | openssh | `systemctl enable sshd` |
| bluetooth | bluez | `systemctl enable bluetooth` |
| PipeWire | pipewire | `systemctl --user enable pipewire` |
| WirePlumber | wireplumber | `systemctl --user enable wireplumber` |
| cups | cups | `systemctl enable cups` |
| avahi | avahi-daemon | `systemctl enable avahi-daemon` |

Unit-файлы лежат в пакете: `/venim/packages/<n>/<v>/lib/systemd/system/*.service`, ссылка в `/usr/lib/systemd/system/`. Свои юниты класть в `/etc/systemd/system/`.

Проверка:

```sh
systemctl status NetworkManager
journalctl -u sshd -b
systemctl --user status pipewire
```

Звук: нужны оба — pipewire и wireplumber. Без wireplumber устройства не переключаются.

## Свои юниты

Положить в `/etc/systemd/system/foo.service`, затем `systemctl daemon-reload && systemctl enable --now foo`.
Логи: `journalctl -u foo -f`.
