# Рецепты venim (.vnb)

Куча готовых рецептов для пакетника `venim` (Exedra Linux):
браузеры, редакторы, терминалы, медиа, офис, чаты, dev-инструменты,
языки, сеть, игры, AI, бэкапы, безопасность.

Рецепты без `sha256` — ставить с отключённой проверкой:

```sh
venim --repo <URL-этого-дерева> update
venim --repo <URL-этого-дерева> search yt
venim --repo <URL-этого-дерева> --no-check install yt-dlp
```

или по одному файлу:

```sh
venim --no-check install ./utils/yt-dlp.vnb
```

`index.json` — готовый индекс для `--repo` (перегенерировать:
`venim index . > index.json`). Полная версия этого репозитория
с доками на английском — в соседней папке `repo/`.
