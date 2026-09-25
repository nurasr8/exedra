# Desktop environments

DE — обычные meta-пакеты, только `depends`. Рецепты: `packages/*/`.

## kde-plasma.vnb

```sh
package=kde-plasma
version=6.2
depends="plasma-desktop plasma-workspace konsole dolphin kate sddm networkmanager pipewire"
```

Ставится `venim install kde-plasma`, DM включается `systemctl enable sddm`.

## gnome.vnb

```sh
package=gnome
version=46
depends="gnome-shell gdm nautilus gnome-terminal networkmanager pipewire"
```

Вход через `gdm`: `systemctl enable gdm`.

## xfce.vnb

```sh
package=xfce
version=4.20
depends="xfce4 xfce4-terminal thunar lightdm networkmanager pipewire"
```

Лёгкий вариант для слабых машин и VM: `venim install xfce`.

Замена компонента — обычным `venim remove/install`, meta-пакет не мешает. Проверка состава: `venim depends kde-plasma`.
