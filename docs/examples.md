# Examples

База, затем окружение на выбор.

## Base

```sh
venim update
venim install base            # kernel glibc systemd bash coreutils
venim install NetworkManager openssh pipewire
systemctl enable NetworkManager sshd
reboot
```

## KDE

```sh
venim install kde-plasma sddm sddm-theme-exedra firefox
systemctl enable sddm
reboot
```

## GNOME

```sh
venim install gnome gdm firefox
systemctl enable gdm
reboot
```

## XFCE

```sh
venim install xfce lightdm firefox
systemctl enable lightdm
reboot
```

## Hyprland / Sway / i3

```sh
venim install hyprland sddm kitty firefox   # или sway, или i3
systemctl enable sddm
# вход через SDDM, сессия выбирается в меню
```

## Софт

```sh
venim install firefox chromium python git
venim install pipewire wireplumber
systemctl --user enable pipewire wireplumber
```
