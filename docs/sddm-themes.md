# SDDM themes

Темы — обычные пакеты, ставит и удаляет `venim`.

## Layout

```
/venim/packages/sddm-theme-exedra/1.0/
  share/sddm/themes/exedra/
    theme.conf
    Main.qml ...
/usr/share/sddm/themes/exedra -> симлинк
```

Пакет описывается рецептом как все остальные: `source=` с tarball темы, sha256 обязателен.

## Интеграция

Симлинк создаёт `venim install`, руками в `/usr` ничего не копировать. Выбор темы в `/etc/sddm.conf`:

```ini
[Theme]
Current=exedra
```

Проверка: `sddm-greeter --test-mode --theme exedra`.

## Своя тема

Скопировать `sddm-theme-exedra` рецепт, поменять `source`, тему, пересобрать `venim build`. Удаление старой темы симлинк чистит сам.
