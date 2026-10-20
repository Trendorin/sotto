# Установка

## Пакеты

В [последнем релизе](https://github.com/Trendorin/sotto/releases/latest) есть пакеты для
Arch Linux, Debian 13+ и Fedora 44+ и AppImage для любого дистрибутива с glibc 2.35+.

```sh
sudo pacman -U ./sotto-*-x86_64.pkg.tar.zst     # Arch, CachyOS, Manjaro
sudo apt install ./sotto_*_amd64.deb             # Debian, Ubuntu
sudo dnf install ./sotto-*.x86_64.rpm            # Fedora
```

Пакет для Arch использует `whisper-cpp` и `ggml-vulkan` из дистрибутива. Пакеты Debian и Fedora
несут свой whisper.cpp 1.9.5 с Vulkan в `/usr/lib/sotto`. В AppImage есть Qt и whisper.cpp; для
видеокарты нужен системный загрузчик Vulkan. При первом запуске AppImage добавляет себя в меню:
порталы выдают горячую клавишу только знакомым приложениям.

Проверить загрузку по `SHA256SUMS` из релиза:

```sh
sha256sum --ignore-missing -c SHA256SUMS
```

## Из исходников

`install.sh` собирает исходники через CMake и ставит в `~/.local`. `sudo` он не использует.

| дистрибутив | зависимости для сборки |
|---|---|
| Arch | `base-devel cmake ninja qt6-base qt6-multimedia qt6-wayland layer-shell-qt whisper-cpp ggml-vulkan wl-clipboard` |
| Debian, Ubuntu | `build-essential cmake ninja-build git qt6-base-dev qt6-multimedia-dev qt6-wayland-dev liblayershellqtinterface-dev libvulkan-dev glslc wl-clipboard` |
| Fedora | `gcc-c++ cmake ninja-build git qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwayland-devel layer-shell-qt-devel vulkan-loader-devel glslc spirv-headers-devel wl-clipboard` |

```sh
./install.sh                  # спросит, куда ставить
./install.sh --yes            # без вопросов, по умолчанию
./install.sh --prefix /opt/sotto
./install.sh --own-whisper    # собрать закреплённый whisper.cpp, даже если он есть в системе
```

Если системного whisper.cpp нет, установщик соберёт закреплённую версию в `<prefix>/lib/sotto`
только для Sotto. В первый раз это занимает несколько минут; с заголовками Vulkan и `glslc`
в сборку войдёт поддержка видеокарты, иначе — только процессор. Если не хватает чего-то ещё,
установщик остановится и покажет команду, которая это поставит в вашем дистрибутиве.

Запущенный Sotto после обновления перезапускается.

### Свой AppImage

```sh
packaging/appimage/build.sh     # без root, docker и podman
```

## Обновление

Скачайте новую версию и снова запустите `./install.sh`. Он увидит установленную версию и
предложит обновить её на месте.

## Удаление

Из приложения: **Настройки → Удалить Sotto…** или `sotto --uninstall`. Из папки с исходниками:

```sh
./install.sh --uninstall           # удалит ровно те файлы, что поставил
./install.sh --uninstall --purge   # и ещё модели и настройки, без вопроса
```

Оба способа удаляют и ярлык, автозапуск и клавишу KDE. Пакеты удаляются менеджером пакетов,
файл AppImage остаётся там, куда вы его сохранили.

## Файлы

| что | где |
|---|---|
| настройки | `~/.config/sotto/` |
| модели | `~/.local/share/sotto/models/` |
| автозапуск, если включён | `~/.config/autostart/io.github.trendorin.Sotto.desktop` |
| список файлов (установка из исходников) | `<prefix>/share/sotto/installed-files` |
