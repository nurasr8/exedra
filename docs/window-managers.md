# Window managers

WM ставятся как обычные пакеты + терминал и лаунчер. Сессии подхватываются SDDM автоматически (`share/wayland-sessions`, `share/xsessions`).

## Hyprland

```sh
venim install hyprland kitty wofi sddm
systemctl enable sddm
```

Зависимости тянутся сами: `wayland, mesa, pipewire, libinput`. Конфиг `~/.config/hypr/hyprland.conf`, пример в `share/doc/hyprland/`.

## Sway

```sh
venim install sway foot wmenu sddm
systemctl enable sddm
```

Sway тише по зависимостям, работает без проприетарных драйверов. Конфиг `~/.config/sway/config`.

## i3

```sh
venim install i3 dmenu rxvt-unicode sddm
systemctl enable sddm
```

X11-вариант. Нужен `xorg-server`, ставится зависимостью i3.

## Openbox

```sh
venim install openbox tint2 lightdm
systemctl enable lightdm
```

Минимальный стек для старых машин. Автозапуск через `~/.config/openbox/autostart`.
