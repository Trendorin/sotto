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
        "en": Flow([[Node("Hotkey", "portal")], [Node("Record", "16 kHz, in RAM")], [Node("Silence cut", "Silero VAD")],
                    [Node("Recognise", "whisper.cpp", accent=True)], [Node("Paste", "portal, Ctrl+V")]]),
        "ru": Flow([[Node("Клавиша", "портал")], [Node("Запись", "16 кГц, в ОЗУ")], [Node("Тишина", "Silero VAD")],
                    [Node("Распознавание", "whisper.cpp", accent=True)], [Node("Вставка", "портал, Ctrl+V")]]),
    },
    flow_alt={
        "en": "Hotkey, recording in memory, silence removed by Silero VAD, whisper.cpp recognises, the text is pasted",
        "ru": "Клавиша, запись в памяти, Silero VAD убирает тишину, whisper.cpp распознаёт, текст вставляется",
    },
    app_icons=["data/io.github.trendorin.Sotto.svg"],
)
