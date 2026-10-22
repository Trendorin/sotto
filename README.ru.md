<div align="center">

<img src=".github/assets/logo.svg" width="88" alt="">

# Sotto

Голосовой ввод для Linux, локально и быстро.

[![Релиз](https://img.shields.io/badge/release-v2.0.0-e0475e)](https://github.com/Trendorin/sotto/releases/latest)
[![Лицензия](https://img.shields.io/badge/license-MIT-e0475e)](LICENSE)
![sherpa-onnx](https://img.shields.io/badge/sherpa--onnx-1.13.8-e0475e)
![whisper.cpp](https://img.shields.io/badge/whisper.cpp-1.9.5-e0475e)
![Платформа](https://img.shields.io/badge/platform-Linux-e0475e)

[Скачать](https://github.com/Trendorin/sotto/releases/latest) · [Установка](#установка) · [Вики](docs/wiki/ru/README.md) · [Изменения](CHANGELOG.md) · [English](README.md)

</div>

<p align="center"><img src=".github/assets/screenshot-ru.png" width="620" alt="Настройки Sotto: GigaAM v3 скачана и готова, источник и журнал загрузки"></p>

Нажмите клавишу, говорите — и текст появляется там, где стоит курсор, фраза за фразой, пока вы
ещё говорите. Sotto (от *sotto voce*, «вполголоса») живёт в трее, слушает только пока вы держите
или переключили клавишу, распознаёт речь на вашем компьютере и печатает результат в активное окно.

## Возможности

- **Ввод во время речи**: каждая фраза распознаётся и печатается, как только вы сделали паузу, а
  не после записи. Или, если хотите, весь текст разом после остановки.
- **Современные модели**: GigaAM v3 — самая точная открытая модель для русского языка — и
  Parakeet TDT v3 для 25 европейских языков, обе с пунктуацией и быстрые на обычном процессоре
  (sherpa-onnx). Whisper — для 99 языков, на видеокарте через Vulkan (whisper.cpp).
- Короткое нажатие — начать и закончить, удержание — запись, пока клавиша зажата.
- Распознавание локальное. Звук остаётся в памяти, не пишется на диск и никуда не отправляется.
- Сделан для Wayland: в KDE, GNOME и COSMIC клавиша и вставка — через порталы рабочего стола, без
  root. В **niri, Hyprland, sway** и других тайлинговых композиторах Sotto показывает бинд,
  который нужно добавить, и вставляет через `wtype`. X11 тоже работает.
- Silero VAD режет речь на фразы, так что тишина в распознавание не попадает и Whisper не
  выдумывает текст на пустой записи.
- Модели скачиваются с Hugging Face, зеркала или вашего сервера, журнал загрузки виден, каждый
  файл сохраняется, только если размер и SHA-256 совпали.
- C++ и виджеты Qt 6 в теме вашей системы. В простое — несколько мегабайт; модель выгружается,
  если ей долго не пользоваться.
- Интерфейс на русском и английском, по языку системы.

## Установка

| ваша система | файл из [последнего релиза](https://github.com/Trendorin/sotto/releases/latest) |
|---|---|
| Arch Linux, CachyOS, Manjaro | `sotto-2.0.0-1-x86_64.pkg.tar.zst` |
| Debian 13 и новее | `sotto_2.0.0-1_amd64.deb` |
| Fedora 44 и новее | `sotto-2.0.0-1.fc44.x86_64.rpm` |
| любой дистрибутив с glibc 2.35+ | `Sotto-2.0.0-x86_64.AppImage` |

```sh
sudo pacman -U ./sotto-2.0.0-1-x86_64.pkg.tar.zst    # Arch
sudo apt install ./sotto_2.0.0-1_amd64.deb            # Debian
sudo dnf install ./sotto-2.0.0-1.fc44.x86_64.rpm      # Fedora
chmod +x Sotto-2.0.0-x86_64.AppImage && ./Sotto-2.0.0-x86_64.AppImage
```

Каждый пакет собирается в чистом контейнере своего дистрибутива, затем ставится и запускается
там до публикации. Проверить файлы: `sha256sum --ignore-missing -c SHA256SUMS`.
В AppImage есть Qt, whisper.cpp с Vulkan и сборки под любой процессор, а также sherpa-onnx; при
первом запуске он добавляет себя в меню — без этого порталы не выдают горячую клавишу.

### Из исходников

`install.sh` проверит CMake, компилятор C++ и Qt 6.5+ с Multimedia и предложит поставить
недостающее через pacman, apt, dnf или zypper. Если в дистрибутиве есть whisper.cpp
(в Arch — `whisper-cpp`), Sotto использует его; иначе установщик соберёт закреплённую версию
только для Sotto, с Vulkan. sherpa-onnx берётся со страницы его релиза и сверяется с закреплённой
SHA-256.

```sh
git clone https://github.com/Trendorin/sotto && cd sotto
./install.sh                 # задаст пару вопросов, соберёт и поставит в ~/.local
./install.sh --uninstall     # удалит ровно то, что поставил
```

`./install.sh --help` покажет параметры (`--yes`, `--prefix`, `--own-whisper`, `--no-sherpa`,
`--no-packages`).

## Начало работы

1. Запустите **Sotto** из меню. В первый раз откроются настройки: скачайте модель
   (**GigaAM v3** для русского, **Parakeet TDT v3** для других европейских языков, **Whisper
   Large v3 Turbo** с видеокартой).
2. Клавиша по умолчанию — **Meta+Space**, поменять её можно кнопкой **Изменить…** в настройках.
   В niri, Hyprland или sway настройки вместо этого покажут строку для вашего конфига.
3. Говорите. Каждая фраза печатается в активное окно, как только вы сделали паузу.

В KDE, GNOME и COSMIC при первой вставке система спросит разрешение на «удалённое управление» —
через этот портал Sotto нажимает Ctrl+V. Отметьте **Запомнить**; без разрешения текст останется в
буфере обмена. Индикатор можно перетащить, клик — остановить, правый клик — отменить.

```sh
sotto --toggle    # начать или закончить; можно повесить на любую клавишу в среде или композиторе
sotto --start | --stop | --cancel | --settings | --uninstall | --quit
sotto --keybind   # какой бинд добавить на этом рабочем столе
sotto --transcribe запись.wav    # распознать запись
```

## Модели

| модель | движок | размер | для чего |
|---|---|---|---|
| **GigaAM v3** | sherpa-onnx | 221 МБ | русский: самая точная, с пунктуацией, быстро на процессоре |
| **Parakeet TDT v3** | sherpa-onnx | 639 МБ | 25 европейских языков, с пунктуацией, быстро на процессоре |
| **Whisper Large v3 Turbo** | whisper.cpp | 547 МБ | 99 языков, точно и быстро на видеокарте |
| Whisper Large v3 Turbo Q8 | whisper.cpp | 834 МБ | чуть точнее, больше памяти |
| Whisper Large v3 | whisper.cpp | 1031 МБ | самый точный Whisper, медленно |
| Whisper Medium / Small / Base | whisper.cpp | 514 / 181 / 57 МБ | меньше и быстрее, но менее точно |

GigaAM v3 ([Сбер](https://github.com/salute-developers/GigaAM), MIT) и Parakeet TDT v3
([NVIDIA](https://huggingface.co/nvidia/parakeet-tdt-0.6b-v3), CC-BY-4.0) берутся в формате
sherpa-onnx с [huggingface.co/csukuangfj](https://huggingface.co/csukuangfj), Whisper — с
[huggingface.co/ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp), Silero VAD — с
[huggingface.co/ggml-org/whisper-vad](https://huggingface.co/ggml-org/whisper-vad) (MIT). Если
Hugging Face недоступен, переключите **Источник** на зеркало [hf-mirror.com](https://hf-mirror.com)
или на свой сервер (**Свой сервер…**). В настройках виден журнал загрузки с точным URL и тем,
откуда файл приходит на самом деле. Свою `ggml-*.bin` или папку модели sherpa-onnx можно
положить в `~/.local/share/sotto/models/`.

## Как устроено

<picture><source media="(prefers-color-scheme: dark)" srcset=".github/assets/flow-ru-dark.svg"><img src=".github/assets/flow-ru-light.svg" width="100%" alt="Клавиша, запись в памяти, Silero VAD режет речь на фразы по паузам, GigaAM, Parakeet или Whisper распознаёт каждую, текст печатается, пока вы говорите"></picture>

Звук пишется в память с частотой 16 кГц. Silero VAD оценивает каждые 32 мс и на паузе
заканчивает фразу; фраза уходит в распознавание в отдельном потоке, пока микрофон продолжает
слушать, и её текст печатается, как только готов. GigaAM и Parakeet работают на ONNX Runtime
(sherpa-onnx), Whisper — на видеокарте (Vulkan) или на лучшем варианте для процессора. Модель
начинает грузиться, пока вы ещё говорите.

## Приватность

Голос не покидает компьютер и не пишется на диск. Единственный выход в сеть — загрузка модели,
которую запускаете вы. Настройки — в `~/.config/sotto/`, модели — в `~/.local/share/sotto/models/`.

Удалить Sotto: **Настройки → Удалить Sotto…** или `sotto --uninstall`. Удаляются программа,
ярлык, автозапуск и клавиша, по желанию — модели и настройки.

## Документация

| | |
|---|---|
| [Установка](docs/wiki/ru/install.md) | пакеты, установщик из исходников, обновление, удаление |
| [Работа с Sotto](docs/wiki/ru/usage.md) | клавиша, ввод во время речи, вставка, индикатор, командная строка |
| [Модели](docs/wiki/ru/models.md) | GigaAM, Parakeet или Whisper, источники, контрольные суммы, свои файлы |
| [Тайлинговые композиторы](docs/wiki/ru/desktops.md) | niri, Hyprland, sway, river, COSMIC: бинды и вставка |
| [Если не работает](docs/wiki/ru/troubleshooting.md) | журнал и частые проблемы |

[Участие](CONTRIBUTING.md) · [Безопасность](SECURITY.md) · [Изменения](CHANGELOG.md)

<div align="center"><sub>MIT · <a href="https://github.com/Trendorin">Trendorin</a></sub></div>
