#!/usr/bin/env bash
# Release notes for version $1: its CHANGELOG.md section plus the download table.
set -euo pipefail
v=${1:?version}
cd "$(dirname "$0")/.."

awk -v v="$v" '$0 ~ "^## \\[" v "\\]" {p=1; next} /^## / && p {exit} p' CHANGELOG.md | sed '/./,$!d'
cat <<NOTES

### Downloads

| File | For |
|---|---|
| \`sotto-$v-1-x86_64.pkg.tar.zst\` | Arch Linux, CachyOS, Manjaro |
| \`sotto_$v-1_amd64.deb\` | Debian 13 and newer |
| \`sotto-$v-1.fc44.x86_64.rpm\` | Fedora 44 and newer |
| \`Sotto-$v-x86_64.AppImage\` | any distribution with glibc 2.35+, no installation |
| \`sotto-$v.tar.gz\` | source code |
| \`SHA256SUMS\` | checksums of all files above |

\`\`\`sh
sudo pacman -U ./sotto-$v-1-x86_64.pkg.tar.zst      # Arch
sudo apt install ./sotto_$v-1_amd64.deb             # Debian
sudo dnf install ./sotto-$v-1.fc44.x86_64.rpm       # Fedora
chmod +x Sotto-$v-x86_64.AppImage && ./Sotto-$v-x86_64.AppImage
\`\`\`

Every package was built in a clean container of its distribution, installed and started there.
Check your download with \`sha256sum --ignore-missing -c SHA256SUMS\`.

Описание и установка по-русски: [README.ru.md](https://github.com/Trendorin/sotto/blob/v$v/README.ru.md).
NOTES
