#!/usr/bin/env python3
"""README art: the flow diagram, the application icon (from logo.svg)
and the social preview, in English and Russian, light and dark.

    python3 tools/art/build.py

Writes into .github/assets/. Needs fonttools, uharfbuzz and rsvg-convert; engine.py
explains the details.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from engine import Flow, Node, build  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]

build(
    ROOT, "Sotto", ("#e0475e", "#ff8a7a"),
    tagline={
        "en": "Voice typing for Linux, local and fast",
        "ru": "Голосовой ввод для Linux, локально и быстро",
    },
    flows={
        "en": Flow([[Node("Hotkey", "portal or compositor")], [Node("Record", "16 kHz, in RAM")], [Node("Phrases", "Silero VAD, at pauses")],
                    [Node("Recognise", "GigaAM or Whisper", accent=True)], [Node("Type", "while you speak")]]),
        "ru": Flow([[Node("Клавиша", "портал или композитор")], [Node("Запись", "16 кГц, в ОЗУ")], [Node("Фразы", "Silero VAD, по паузам")],
                    [Node("Распознавание", "GigaAM или Whisper", accent=True)], [Node("Ввод", "пока вы говорите")]]),
    },
    flow_alt={
        "en": "Hotkey, recording in memory, Silero VAD cuts phrases at the pauses, GigaAM, Parakeet or Whisper recognises each one, the text is typed while you speak",
        "ru": "Клавиша, запись в памяти, Silero VAD режет речь на фразы по паузам, GigaAM, Parakeet или Whisper распознаёт каждую, текст печатается, пока вы говорите",
    },
    app_icons=["packaging/io.github.trendorin.Sotto.svg"],
)
