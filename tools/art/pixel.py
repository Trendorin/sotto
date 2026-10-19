"""Pixel primitives for the README art, taken from github.com/Trendorin/Trendorin build.py
(5x7 font, 12-row BIG font, notched dark screens, header, labels, readout and cards) and
the extra glyphs of github.com/Trendorin/luci-theme-pixel (Cyrillic, arrows, symbols).
Kept verbatim so vizor looks like the rest of the profile; vizor-specific art is in build.py."""
from __future__ import annotations

import html
import random
from collections import defaultdict

# ── Palette ──────────────────────────────────────────────────────────────────
# Every SVG ships in two variants and README switches them with <picture>.
THEMES = {
    "dark":  {"ink": "#ededed", "mid": "#8e8e8e", "dim": "#4a4a4a"},
    "light": {"ink": "#141414", "mid": "#6f6f6f", "dim": "#c4c4c4"},
}

# ── 5x7 bitmap font ("#" = pixel; narrow glyphs are allowed) ─────────────────
FONT = {
    "A": ".###. #...# #...# ##### #...# #...# #...#",
    "B": "####. #...# #...# ####. #...# #...# ####.",
    "C": ".###. #...# #.... #.... #.... #...# .###.",
    "D": "####. #...# #...# #...# #...# #...# ####.",
    "E": "##### #.... #.... ####. #.... #.... #####",
    "F": "##### #.... #.... ####. #.... #.... #....",
    "G": ".###. #...# #.... #.### #...# #...# .###.",
    "H": "#...# #...# #...# ##### #...# #...# #...#",
    "I": ".###. ..#.. ..#.. ..#.. ..#.. ..#.. .###.",
    "J": "..### ...#. ...#. ...#. ...#. #..#. .##..",
    "K": "#...# #..#. #.#.. ##... #.#.. #..#. #...#",
    "L": "#.... #.... #.... #.... #.... #.... #####",
    "M": "#...# ##.## #.#.# #...# #...# #...# #...#",
    "N": "#...# #...# ##..# #.#.# #..## #...# #...#",
    "O": ".###. #...# #...# #...# #...# #...# .###.",
    "P": "####. #...# #...# ####. #.... #.... #....",
    "Q": ".###. #...# #...# #...# #.#.# #..#. .##.#",
    "R": "####. #...# #...# ####. #.#.. #..#. #...#",
    "S": ".#### #.... #.... .###. ....# ....# ####.",
    "T": "##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#..",
    "U": "#...# #...# #...# #...# #...# #...# .###.",
    "V": "#...# #...# #...# #...# #...# .#.#. ..#..",
    "W": "#...# #...# #...# #.#.# #.#.# ##.## #...#",
    "X": "#...# #...# .#.#. ..#.. .#.#. #...# #...#",
    "Y": "#...# #...# .#.#. ..#.. ..#.. ..#.. ..#..",
    "Z": "##### ....# ...#. ..#.. .#... #.... #####",
    "0": ".###. #...# #..## #.#.# ##..# #...# .###.",
    "1": "..#.. .##.. ..#.. ..#.. ..#.. ..#.. .###.",
    "2": ".###. #...# ....# ...#. ..#.. .#... #####",
    "3": "##### ...#. ..#.. ...#. ....# #...# .###.",
    "4": "...#. ..##. .#.#. #..#. ##### ...#. ...#.",
    "5": "##### #.... ####. ....# ....# #...# .###.",
    "6": "..##. .#... #.... ####. #...# #...# .###.",
    "7": "##### ....# ...#. ..#.. .#... .#... .#...",
    "8": ".###. #...# #...# .###. #...# #...# .###.",
    "9": ".###. #...# #...# .#### ....# ...#. .##..",
    " ": "... ... ... ... ... ... ...",
    ".": ". . . . . . #",
    ",": ".. .. .. .. .. .# #.",
    ":": ". . # . . # .",
    "!": "# # # # # . #",
    "?": ".###. #...# ....# ...#. ..#.. ..... ..#..",
    "'": "# # . . . . .",
    "-": "... ... ... ### ... ... ...",
    "_": "..... ..... ..... ..... ..... ..... #####",
    "+": "... ... .#. ### .#. ... ...",
    "=": "..... ..... ##### ..... ##### ..... .....",
    "/": "....# ....# ...#. ..#.. .#... #.... #....",
    "|": "# # # # # # #",
    "(": "..# .#. #.. #.. #.. .#. ..#",
    ")": "#.. .#. ..# ..# ..# .#. #..",
    "[": "### #.. #.. #.. #.. #.. ###",
    "]": "### ..# ..# ..# ..# ..# ###",
    "<": "...# ..#. .#.. #... .#.. ..#. ...#",
    ">": "#... .#.. ..#. ...# ..#. .#.. #...",
    "#": ".#.#. .#.#. ##### .#.#. ##### .#.#. .#.#.",
    "*": "..... ..#.. #.#.# .###. #.#.# ..#.. .....",
    "&": ".##.. #..#. #.#.. .#... #.#.# #..#. .##.#",
    "@": ".###. #...# #.### #.#.# #.### #.... .###.",
    "·": ". . . # . . .",
    # lowercase: x-height rows 2-6, descenders in rows 7-8
    "a": "..... ..... .###. ....# .#### #...# .#### ..... .....",
    "b": "#.... #.... ####. #...# #...# #...# ####. ..... .....",
    "c": "..... ..... .###. #.... #.... #.... .###. ..... .....",
    "d": "....# ....# .#### #...# #...# #...# .#### ..... .....",
    "e": "..... ..... .###. #...# ##### #.... .###. ..... .....",
    "f": "..## .#.. #### .#.. .#.. .#.. .#.. .... ....",
    "g": "..... ..... .#### #...# #...# #...# .#### ....# .###.",
    "h": "#.... #.... ####. #...# #...# #...# #...# ..... .....",
    "i": ".#. ... ##. .#. .#. .#. ### ... ...",
    "j": "...# .... ..## ...# ...# ...# ...# #..# .##.",
    "k": "#... #... #..# #.#. ##.. #.#. #..# .... ....",
    "l": "##. .#. .#. .#. .#. .#. ### ... ...",
    "m": "..... ..... ##.#. #.#.# #.#.# #.#.# #.#.# ..... .....",
    "n": "..... ..... ####. #...# #...# #...# #...# ..... .....",
    "o": "..... ..... .###. #...# #...# #...# .###. ..... .....",
    "p": "..... ..... ####. #...# #...# #...# ####. #.... #....",
    "q": "..... ..... .#### #...# #...# #...# .#### ....# ....#",
    "r": "..... ..... #.##. ##..# #.... #.... #.... ..... .....",
    "s": "..... ..... .#### #.... .###. ....# ####. ..... .....",
    "t": ".#.. .#.. #### .#.. .#.. .#.. ..## .... ....",
    "u": "..... ..... #...# #...# #...# #...# .#### ..... .....",
    "v": "..... ..... #...# #...# #...# .#.#. ..#.. ..... .....",
    "w": "..... ..... #...# #...# #.#.# #.#.# .#.#. ..... .....",
    "x": "..... ..... #...# .#.#. ..#.. .#.#. #...# ..... .....",
    "y": "..... ..... #...# #...# #...# #...# .#### ....# .###.",
    "z": "..... ..... ##### ...#. ..#.. .#... ##### ..... .....",
}

EXTRA = {
    # ── punctuation and symbols the panel uses ───────────────────────────────
    "%": "##..# ##.#. ...#. ..#.. .#... .#.## #..##",
    ";": ".. .. .# .. .. .# #.",
    '"': "#.# #.# ... ... ... ... ...",
    "$": "..#.. .#### #.#.. .###. ..#.# ####. ..#..",
    "^": "..#.. .#.#. #...# ..... ..... ..... .....",
    "~": "..... ..... .#... #.#.# ...#. ..... .....",
    "`": "#. .# .. .. .. .. ..",
    "{": "..# .#. .#. #.. .#. .#. ..#",
    "}": "#.. .#. .#. ..# .#. .#. #..",
    "\\": "#.... #.... .#... ..#.. ...#. ....# ....#",
    "°": ".#. #.# .#. ... ... ... ...",
    "×": "..... #...# .#.#. ..#.. .#.#. #...# .....",
    "±": "..#.. ..#.. ##### ..#.. ..#.. ..... #####",
    "→": "..... ..#.. ...#. ##### ...#. ..#.. .....",
    "←": "..... ..#.. .#... ##### .#... ..#.. .....",
    "↑": "..#.. .###. #.#.# ..#.. ..#.. ..#.. ..#..",
    "↓": "..#.. ..#.. ..#.. ..#.. #.#.# .###. ..#..",
    "↗": "..##### .....## ....#.# ...#..# ..#.... .#..... #......",
    "…": "..... ..... ..... ..... ..... ..... #.#.#",
    "–": ".... .... .... #### .... .... ....",
    "—": "...... ...... ...... ###### ...... ...... ......",
    "•": "... ... .#. ### .#. ... ...",
    "«": "..... ..#.# .#.#. #.#.. .#.#. ..#.# .....",
    "»": "..... #.#.. .#.#. ..#.# .#.#. #.#.. .....",
    "✓": "..... ....# ...#. #.#.. .#... ..... .....",
    "▲": "..... ..#.. .###. ##### ..... ..... .....",
    "▼": "..... ##### .###. ..#.. ..... ..... .....",
    "№": "#..#.. ##.#.. #.##.. #..#.# #..#.. #..#.# ......",
    "©": ".###. #...# #.#.# ##..# #.#.# #...# .###.",
    "|": "# # # # # # #",
    # ── Cyrillic capitals (А В Е К М Н О Р С Т Х reuse the Latin shapes) ──────
    "Б": "##### #.... #.... ####. #...# #...# ####.",
    "Г": "##### #.... #.... #.... #.... #.... #....",
    "Д": "..##. .#.#. .#.#. .#.#. .#.#. ##### #...#",
    "Ж": "#.#.# #.#.# .###. ..#.. .###. #.#.# #.#.#",
    "З": ".###. #...# ....# ..##. ....# #...# .###.",
    "И": "#...# #...# #..## #.#.# ##..# #...# #...#",
    "Й": ".#.#. ..#.. #...# #..## #.#.# ##..# #...#",
    "Л": "..### .#..# .#..# .#..# .#..# .#..# #...#",
    "П": "##### #...# #...# #...# #...# #...# #...#",
    "У": "#...# #...# #...# .#### ....# #...# .###.",
    "Ф": "..#.. .###. #.#.# #.#.# #.#.# .###. ..#..",
    "Ц": "#..#. #..#. #..#. #..#. #..#. ##### ....#",
    "Ч": "#...# #...# #...# .#### ....# ....# ....#",
    "Ш": "#.#.# #.#.# #.#.# #.#.# #.#.# #.#.# #####",
    "Щ": "#.#.#. #.#.#. #.#.#. #.#.#. #.#.#. ###### .....#",
    "Ъ": "##... .#... .#... .###. .#..# .#..# .###.",
    "Ы": "#...# #...# #...# ###.# #.#.# #.#.# ###.#",
    "Ь": "#.... #.... #.... ####. #...# #...# ####.",
    "Э": ".###. #...# ....# .#### ....# #...# .###.",
    "Ю": "#..#. #.#.# #.#.# ###.# #.#.# #.#.# #..#.",
    "Я": ".#### #...# #...# .#### ..#.# .#..# #...#",
    "Ё": "^.#.#. ##### #.... #.... ####. #.... #.... #####",
    # ── Cyrillic lowercase (а е о р с у х reuse the Latin shapes) ─────────────
    "б": "..### .#... #.... ####. #...# #...# .###. ..... .....",
    "в": "..... ..... ####. #...# ####. #...# ####. ..... .....",
    "г": "..... ..... ##### #.... #.... #.... #.... ..... .....",
    "д": "..... ..... ..##. .#.#. .#.#. .#.#. ##### #...# .....",
    "ж": "..... ..... #.#.# #.#.# .###. #.#.# #.#.# ..... .....",
    "з": "..... ..... ####. ....# .###. ....# ####. ..... .....",
    "и": "..... ..... #...# #..## #.#.# ##..# #...# ..... .....",
    "й": ".#.#. ..#.. #...# #..## #.#.# ##..# #...# ..... .....",
    "к": ".... .... #..# #.#. ##.. #.#. #..# .... ....",
    "л": "..... ..... ..### .#..# .#..# .#..# #...# ..... .....",
    "м": "..... ..... #...# ##.## #.#.# #...# #...# ..... .....",
    "н": "..... ..... #...# #...# ##### #...# #...# ..... .....",
    "п": "..... ..... ##### #...# #...# #...# #...# ..... .....",
    "т": "..... ..... ##### ..#.. ..#.. ..#.. ..#.. ..... .....",
    "ф": "..... ..#.. .###. #.#.# #.#.# #.#.# .###. ..#.. .....",
    "ц": "..... ..... #..#. #..#. #..#. #..#. ##### ....# .....",
    "ч": "..... ..... #...# #...# .#### ....# ....# ..... .....",
    "ш": "..... ..... #.#.# #.#.# #.#.# #.#.# ##### ..... .....",
    "щ": "...... ...... #.#.#. #.#.#. #.#.#. #.#.#. ###### .....# ......",
    "ъ": "..... ..... ##... .#... .###. .#..# .###. ..... .....",
    "ы": "..... ..... #...# #...# ###.# #.#.# ###.# ..... .....",
    "ь": ".... .... #... #... ###. #..# ###. .... ....",
    "э": "..... ..... .###. ....# .#### ....# .###. ..... .....",
    "ю": "..... ..... #..#. #.#.# ###.# #.#.# #..#. ..... .....",
    "я": "..... ..... .#### #...# .#### .#..# #...# ..... .....",
    "ё": ".#.#. ..... .###. #...# ##### #.... .###. ..... .....",
}

# Characters drawn exactly like another one
ALIASES = {
    "А": "A", "В": "B", "Е": "E", "К": "K", "М": "M", "Н": "H", "О": "O", "Р": "P", "С": "C", "Т": "T", "Х": "X",
    "а": "a", "е": "e", "о": "o", "р": "p", "с": "c", "у": "y", "х": "x",
    "‐": "-", "‑": "-", "−": "-",          # hyphen, non-breaking hyphen, minus
    "‘": "'", "’": "'", "“": '"', "”": '"', "„": '"',
    "·": "·", "∙": "·", "•": "•",
    "✔": "✓", "✘": "×", "✕": "×", "✗": "×",
    " ": " ",
}

FONT.update({k: v for k, v in EXTRA.items() if not v.startswith("^")})
FONT.update({k: FONT[v] for k, v in ALIASES.items() if v in FONT and k not in FONT})
FONT["Ё"] = FONT["E"]  # no room above the cap height in a 7-row line

GLYPHS = {k: v.split() for k, v in FONT.items()}
LINE = 9  # rows a mixed-case line needs, descenders included
UNKNOWN: set[str] = set()

ARROW = "..##### .....## ....#.# ...#..# ..#.... .#..... #......"  # 7x7 ↗

# 20x20 icons for the cards; build.py fills it
LOGOS: dict[str, str] = {}

def esc(s: str) -> str:
    return html.escape(str(s), quote=True)


def slug(s: str) -> str:
    s = s.lower().replace("+", "p").replace("#", "sharp")  # C++ → cpp, C# → csharp
    return re.sub(r"[^a-z0-9]+", "-", s).strip("-")


# ── Pixel geometry ───────────────────────────────────────────────────────────
def text_layout(text: str, upper=True):
    """Per-character (x offset, pixels) and total width, in font units."""
    chars, x = [], 0
    for ch in text.upper() if upper else text:
        rows = GLYPHS.get(ch) or GLYPHS.get(ch.upper())
        if rows is None:
            UNKNOWN.add(ch)
            rows = GLYPHS["?"]
        pts = [(cx, cy) for cy, row in enumerate(rows) for cx, c in enumerate(row) if c == "#"]
        chars.append((x, pts))
        x += len(rows[0]) + 1
    return chars, max(x - 1, 0)


def text_pixels(text: str, upper=True):
    chars, width = text_layout(text, upper)
    return [(x0 + x, y) for x0, pts in chars for x, y in pts], width


def bitmap(rows: str):
    return [(x, y) for y, row in enumerate(rows.split()) for x, c in enumerate(row) if c == "#"]


def runs(pixels):
    """Merge unit pixels into horizontal runs (x, y, length)."""
    by_row = defaultdict(set)
    for x, y in pixels:
        by_row[y].add(x)
    out = []
    for y in sorted(by_row):
        xs = sorted(by_row[y])
        start = prev = xs[0]
        for x in xs[1:]:
            if x != prev + 1:
                out.append((start, y, prev - start + 1))
                start = x
            prev = x
        out.append((start, y, prev - start + 1))
    return out


def pixel_rects(pixels, s, ox=0, oy=0):
    return [(ox + x * s, oy + y * s, n * s, s) for x, y, n in runs(pixels)]


def d_rects(rects) -> str:
    return "".join(f"M{x} {y}h{w}v{h}h{-w}z" for x, y, w, h in rects)


def frame_rects(x, y, w, h, t=2, bottom=None):
    """Outline with notched (pixel-rounded) corners; optional thicker bottom edge."""
    b = bottom or t
    return [(x + t, y, w - 2 * t, t), (x + t, y + h - b, w - 2 * t, b),
            (x, y + t, t, h - t - b), (x + w - t, y + t, t, h - t - b)]


class Svg:
    def __init__(self, w, h, theme, label):
        self.w, self.h, self.theme, self.label = w, h, theme, label
        self.css, self.defs, self.body = [], [], []

    def rects(self, rects, cls, extra=""):
        if rects:
            self.body.append(f'<path class="{cls}"{extra} d="{d_rects(rects)}"/>')

    def __str__(self):
        css = ""
        if self.theme:
            t = THEMES[self.theme]
            css = f".i{{fill:{t['ink']}}}.m{{fill:{t['mid']}}}.d{{fill:{t['dim']}}}"
        if self.css:
            css += "".join(self.css) + "@media (prefers-reduced-motion:reduce){*{animation:none!important}}"
        return (
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
            f'viewBox="0 0 {self.w} {self.h}" shape-rendering="crispEdges" role="img" '
            f'aria-label="{esc(self.label)}"><title>{esc(self.label)}</title>'
            + (f"<style>{css}</style>" if css else "")
            + (f"<defs>{''.join(self.defs)}</defs>" if self.defs else "")
            + "".join(self.body) + "</svg>\n"
        )


# ── Pieces ───────────────────────────────────────────────────────────────────
# Animations only ever hide things *while running*, so renderers that ignore
# CSS animation (or prefers-reduced-motion) show the finished frame.
ANIM = ("@keyframes h{from,to{opacity:0}}@keyframes b{50%{opacity:0}}"
        ".h{animation-name:h;animation-timing-function:steps(1,end)}")


# 12-row squared display font for the header wordmark. "*n" repeats a row.
BIG = {
    "A": ".########. ########## ##......##*3 ##########*2 ##......##*5",
    "B": "#########. ########## ##......##*3 #########.*2 ##......##*3 ########## #########.",
    "C": ".######### ########## ##........*8 ########## .#########",
    "D": "#########. ########## ##......##*8 ########## #########.",
    "E": "#########*2 ##.......*3 #######..*2 ##.......*3 #########*2",
    "F": "#########*2 ##.......*3 #######..*2 ##.......*5",
    "G": ".######### ########## ##........*3 ##...#####*2 ##......##*3 ########## .#########",
    "H": "##......##*5 ##########*2 ##......##*5",
    "I": "##*12",
    "J": ".......##*9 ##.....## ######### .#######.",
    "K": "##......## ##.....##. ##....##.. ##...##... ##..##.... #####.....*2 ##..##.... ##...##... "
         "##....##.. ##.....##. ##......##",
    "L": "##.......*10 #########*2",
    "M": "###......### ####....#### ##.##..##.## ##..####..## ##...##...## ##........##*7",
    "N": "###.....##*2 ####....##*2 ##.##...##*2 ##..##..##*2 ##...##.##*2 ##....####*2",
    "O": ".########. ########## ##......##*8 ########## .########.",
    "P": "#########. ########## ##......##*3 ########## #########. ##........*5",
    "Q": ".########. ########## ##......##*6 ##...##.## ##....#### ########## .#######.#",
    "R": "#########. ########## ##......##*3 ########## #########. ##...##... ##....##.. ##.....##. ##......##*2",
    "S": ".######### ########## ##........*3 #########. .######### ........##*3 ########## #########.",
    "T": "##########*2 ....##....*10",
    "U": "##......##*10 ########## .########.",
    "V": "##......##*8 .##....##. ..##..##.. ...####... ....##....",
    "W": "##........##*7 ##...##...## ##..####..## ##.##..##.## ####....#### ###......###",
    "X": "##......##*2 .##....##. ..##..##.. ...####... ....##....*2 ...####... ..##..##.. .##....##. ##......##*2",
    "Y": "##......##*2 .##....##. ..##..##.. ...####... ....##....*7",
    "Z": "##########*2 .......##. ......##.. .....##... ....##.... ...##..... ..##...... .##....... ##........ ##########*2",
    "0": ".########. ########## ##......##*3 ##..##..##*2 ##......##*3 ########## .########.",
    "1": "####.. ####.. ..##..*8 ######*2",
    "2": "#########. ########## ........##*3 .######### #########. ##........*3 ##########*2",
    "3": "#########. ########## ........##*3 ..########*2 ........##*3 ########## #########.",
    "4": "##......##*5 ##########*2 ........##*5",
    "5": "##########*2 ##........*3 #########. ########## ........##*3 ########## #########.",
    "6": ".######### ########## ##........*3 #########. ########## ##......##*3 ########## .########.",
    "7": "##########*2 ........##*10",
    "8": ".########. ########## ##......##*3 .########. ########## ##......##*3 ########## .########.",
    "9": ".########. ########## ##......##*3 ########## .######### ........##*3 ########## #########.",
    " ": ".....*12",
    "-": "......*5 ######*2 ......*5",
    ".": "..*10 ##*2",
}
BIG_GLYPHS = {k: [row for tok in v.split() for row in [tok.split("*")[0]] * int((tok.split("*") + ["1"])[1])]
              for k, v in BIG.items()}
assert all(len(g) == 12 and len({len(r) for r in g}) == 1 for g in BIG_GLYPHS.values()), "BIG glyphs must be 12 equal rows"

# The header is a dark "screen" in both GitHub themes; only its frame differs.
SCREEN = {"bg": "#0a0a0a", "hi": "#e4e4e4", "lo": "#8c8c8c", "shadow": "#1d1d1d", "noise": "#4e4e4e",
          "rule": "#333333", "sub": "#7a7a7a", "value": "#a6a6a6", "cursor": "#e4e4e4",
          "art": ("#3a3a3a", "#8c8c8c", "#e4e4e4"), "primary": "#5c5c5c",
          "dust": ("#1f1f1f", "#2f2f2f", "#4a4a4a"), "frame": {"dark": "#222222", "light": "#0a0a0a"}}


def big_layout(text, gap=2):
    """Per-character (x offset, width, pixels) and total width for the BIG font, in units."""
    chars, x = [], 0
    for ch in text.upper():
        rows = BIG_GLYPHS.get(ch)
        if rows is None:
            UNKNOWN.add(ch)
            rows = BIG_GLYPHS[" "]
        pts = [(cx, cy) for cy, row in enumerate(rows) for cx, c in enumerate(row) if c == "#"]
        chars.append((x, len(rows[0]), pts))
        x += len(rows[0]) + gap
    return chars, max(x - gap, 0)


def notched(x, y, w, h):
    """Rectangle with two-step pixel-rounded corners."""
    return [(x + 4, y, w - 8, h), (x + 2, y + 2, w - 4, h - 4), (x, y + 4, w, h - 8)]


def header_svg(title, subtitle, theme, W=764, H=248):  # 764 = two 380px cards + a space
    """Dark boot screen. The wordmark decodes out of pixel noise letter by letter,
    the subtitle types itself, then a glint sweeps the title every few seconds."""
    c = SCREEN
    rnd = random.Random(title + subtitle)  # deterministic: same config, same SVG
    chars, tw = big_layout(title)
    u = max(3, min(6, (W - 160) // max(tw, 1)))
    tx, ty, tww, th = (W - tw * u) // 2, 62, tw * u, 12 * u
    split = 7  # rows above are bright, rows below dim: a two-tone "chrome" cut
    uy = ty + th + 20
    sub, sw = text_layout("> " + subtitle)
    sx, sy = (W - (sw + 2 + 5) * 2) // 2, uy + 18

    # timeline, seconds
    frame_t, frames = 0.07, 5
    starts = [0.35 + i * 0.12 for i in range(len(chars))]
    settle = [t + frame_t * frames for t in starts]
    t_line = (settle[-1] if settle else 0.4) + 0.08
    t_sub = t_line + 0.5
    t_ready = t_sub + len(sub) * 0.03 + 0.05
    sweep = tww + 16 * u
    glint_steps = sweep // u

    svg = Svg(W, H, None, f"{title}: {subtitle}")
    svg.css.append(
        f".bg{{fill:{c['bg']}}}.fr{{fill:{c['frame'][theme]}}}.hi{{fill:{c['hi']}}}.lo{{fill:{c['lo']}}}"
        f".sh{{fill:{c['shadow']}}}.hd{{fill:{c['rule']}}}.sb{{fill:{c['sub']}}}"
        f".cu{{fill:{c['cursor']}}}.gl{{fill:#fff}}"
        + "".join(f".d{i}{{fill:{col}}}" for i, col in enumerate(c["dust"]))
        + f".nz{{fill:{c['noise']};opacity:0;animation:s {frame_t}s}}"
        "@keyframes h{from,to{opacity:0}}@keyframes s{from,to{opacity:1}}@keyframes b{50%{opacity:0}}"
        "@keyframes tw{50%{opacity:.12}}@keyframes ul{from{transform:scaleX(0)}}"
        f"@keyframes gl{{0%{{transform:translateX(0);animation-timing-function:steps({glint_steps},end)}}"
        f"14%,100%{{transform:translateX({sweep}px)}}}}"
        ".h{animation-name:h;animation-timing-function:steps(1,end)}"
        ".tw{animation:tw 5s steps(3,end) infinite}"
        f".ul{{transform-box:fill-box;transform-origin:center;animation:ul .45s steps(9,end) {t_line:.2f}s backwards}}"
        f".gb{{animation:gl 7s {t_ready + 0.4:.2f}s infinite}}"
    )

    # screen with a thin frame
    svg.rects(notched(0, 0, W, H), "fr")
    svg.rects(notched(2, 2, W - 4, H - 4), "bg")

    # star dust, kept away from the text blocks
    busy = [(tx - 28, ty - 18, tx + tww + 28, sy + 22)]
    for _ in range(22):
        size = rnd.choice((2, 2, 2, 4))
        x, y = rnd.randrange(14, W - 18, 2), rnd.randrange(12, H - 16, 2)
        if any(x0 <= x <= x1 and y0 <= y <= y1 for x0, y0, x1, y1 in busy):
            continue
        tone = rnd.choice((0, 0, 1, 1, 2))
        twinkle = rnd.random() < 0.35
        extra = f' style="animation-delay:-{rnd.uniform(0, 5):.2f}s"' if twinkle else ""
        svg.rects([(x, y, size, size)], f"d{tone}" + (" tw" if twinkle else ""), extra)

    # wordmark: noise frames, then shadow + two-tone letters, each slot settling in turn
    letters = []
    for (x0, w, pts), t0, t1 in zip(chars, starts, settle):
        ox = tx + x0 * u
        for k in range(frames):
            noise = [(x, y) for y in range(12) for x in range(w) if rnd.random() < 0.3]
            svg.rects(pixel_rects(noise, u, ox, ty), "nz", f' style="animation-delay:{t0 + k * frame_t:.2f}s"')
        dur = f' style="animation-duration:{t1:.2f}s"'
        svg.rects(pixel_rects(pts, u, ox + u, ty + u), "sh h", dur)
        svg.rects(pixel_rects([p for p in pts if p[1] < split], u, ox, ty), "hi h", dur)
        svg.rects(pixel_rects([p for p in pts if p[1] >= split], u, ox, ty), "lo h", dur)
        letters += pixel_rects(pts, u, ox, ty)

    # glint: a slanted pixel band clipped to the letters
    band = [(tx - 8 * u + ((11 - r) // 2) * u, ty + r * u, 2 * u, u) for r in range(12)]  # starts off the letters
    svg.defs.append(f'<clipPath id="t"><path d="{d_rects(letters)}"/></clipPath>')
    svg.body.append(f'<g clip-path="url(#t)"><path class="gl gb" d="{d_rects(band)}"/></g>')

    # underline that opens from the centre, with bright end caps
    svg.body.append(f'<g class="ul"><path class="hd" d="{d_rects([(tx, uy, tww, 2)])}"/>'
                    f'<path class="hi" d="{d_rects([(tx, uy - 1, 4, 4), (tx + tww - 4, uy - 1, 4, 4)])}"/></g>')

    # typed subtitle and cursor
    for i, (x0, pts) in enumerate(sub):
        if pts:
            svg.rects(pixel_rects([(x0 + x, y) for x, y in pts], 2, sx, sy), ("cu" if i == 0 else "sb") + " h",
                      f' style="animation-duration:{t_sub + i * 0.03:.2f}s"')
    svg.body.append(f'<rect class="cu" x="{sx + (sw + 2) * 2}" y="{sy}" width="10" height="14" '
                    f'style="animation:h {t_ready:.2f}s steps(1,end),b 1.1s steps(1,end) {t_ready:.2f}s infinite"/>')

    # slow CRT refresh band rolling down the screen
    svg.css.append(f"@keyframes rf{{0%{{transform:translateY(0);animation-timing-function:steps({H // 4},end)}}"
                   f"30%,100%{{transform:translateY({H}px)}}}}.rf{{animation:rf 9s {t_ready:.2f}s infinite}}")
    svg.body.append(f'<rect class="rf" x="2" y="-10" width="{W - 4}" height="10" fill="#fff" fill-opacity=".035"/>')

    # CRT scanlines over everything
    svg.defs.append(f'<pattern id="sl" width="4" height="3" patternUnits="userSpaceOnUse">'
                    f'<rect y="2" width="4" height="1" fill="#000" fill-opacity=".3"/></pattern>')
    svg.body.append(f'<path fill="url(#sl)" d="{d_rects(notched(2, 2, W - 4, H - 4))}"/>')
    return svg


def label_svg(text, theme, num=None, rule=72, s=2):
    """Section label between short dotted rules: ···· 01 / ABOUT ····.
    Kept narrow so phones show it at 1:1 instead of shrinking it."""
    parts = ([(f"{num:02d}", "m"), ("/", "d")] if num is not None else []) + [(text, "i" if num is not None else "d")]
    placed, x = [], 0
    for chunk, cls in parts:
        px, w = text_pixels(chunk)
        placed.append((x, px, cls))
        x += w + 4
    tw = (x - 4) * s
    gap, top = 18, 16
    W, H = 2 * (rule + gap) + tw, 7 * s + 24
    svg = Svg(W, H, theme, text)
    for xu, px, cls in placed:
        svg.rects(pixel_rects([(xu + a, b) for a, b in px], s, rule + gap, top), cls)
    dy = top + 3 * s
    dots = [(x, dy, 2, 2) for x in range(0, rule, 6)]
    dots += [(W - 2 - x, dy, 2, 2) for x in range(0, rule, 6)]
    svg.rects(dots, "d")
    return svg


def screen(svg, W, H, theme, extra_css=""):
    """Dark card body shared by every block: notched frame, background, colour classes."""
    c = SCREEN
    svg.css.append(
        f".bg{{fill:{c['bg']}}}.fr{{fill:{c['frame'][theme]}}}.pr{{fill:{c['primary']}}}.hi{{fill:{c['hi']}}}"
        f".lo{{fill:{c['lo']}}}.hd{{fill:{c['rule']}}}.sb{{fill:{c['sub']}}}.va{{fill:{c['value']}}}"
        + "".join(f".a{i + 1}{{fill:{col}}}" for i, col in enumerate(c["art"]))
        + "@keyframes h{from,to{opacity:0}}.h{animation-name:h;animation-timing-function:steps(1,end)}" + extra_css)
    svg.rects(notched(0, 0, W, H), "fr")
    svg.rects(notched(2, 2, W - 4, H - 4), "bg")


def scanlines(svg, W, H):
    svg.defs.append('<pattern id="sl" width="4" height="3" patternUnits="userSpaceOnUse">'
                    '<rect y="2" width="4" height="1" fill="#000" fill-opacity=".3"/></pattern>')
    svg.body.append(f'<path fill="url(#sl)" d="{d_rects(notched(2, 2, W - 4, H - 4))}"/>')


def wrap(text, width_px, s=2):
    """Greedy word wrap measured in pixel-font units."""
    lines, line = [], ""
    for word in text.split():
        trial = f"{line} {word}".strip()
        if line and text_layout(trial, upper=False)[1] * s > width_px:
            lines.append(line)
            line = word
        else:
            line = trial
    return lines + [line] if line else lines


def portrait_svg(grid, theme, label, W=300, H=320, gap=8):
    """Dithered portrait (4 tones) on the dark screen, scanned in top to bottom.
    `gap` keeps it off the readout when the two stack on phones."""
    n = len(grid)
    cell = max(1, min(W - 40, H - 40) // n)
    ox, oy = (W - n * cell) // 2, (H - n * cell) // 2
    svg = Svg(W, H + gap, None, label)
    screen(svg, W, H, theme)
    for band in range(0, n, 4):
        dur = f' style="animation-duration:{0.25 + band / 4 * 0.05:.2f}s"'
        for tone in (1, 2, 3):
            px = [(x, y) for y in range(band, min(band + 4, n)) for x in range(n) if grid[y][x] == tone]
            svg.rects(pixel_rects(px, cell, ox, oy), f"a{tone} h", dur)
    scanlines(svg, W, H)
    return svg


def spec_svg(user, host, rows, theme, W=460, H=None, s=2, gap=8):
    """neofetch-style readout: user@host, a rule, key/value rows and a gray ramp."""
    pad, pitch = 24, LINE * s + 4
    keyw = max(text_layout(k, upper=False)[1] for k, _ in rows) * s + 18
    lines = []
    for key, value in rows:
        for i, part in enumerate(wrap(value, W - 2 * pad - keyw, s)):
            lines.append((key if i == 0 else "", part))
    content = 62 + len(lines) * pitch + 16 + 12
    H = max(H or 0, content + pad)
    y0 = (H - content - pad) // 2  # centre the block when the card is taller than it needs
    svg = Svg(W, H + gap, None, f"{user}@{host}: " + "; ".join(f"{k} {v}" for k, v in rows))
    screen(svg, W, H, theme)

    x = pad
    for chunk, cls in ((user, "hi"), ("@", "sb"), (host, "hi")):
        px, w = text_pixels(chunk, upper=False)
        svg.rects(pixel_rects(px, s, x, pad + y0), f"{cls} h", ' style="animation-duration:.2s"')
        x += (w + 1) * s
    svg.rects([(pad, pad + y0 + 24, x - pad - s, 2)], "hd h", ' style="animation-duration:.3s"')
    for i, (key, value) in enumerate(lines):
        y = 62 + y0 + i * pitch
        dur = f' style="animation-duration:{0.4 + i * 0.09:.2f}s"'
        if key:
            svg.rects(pixel_rects(text_pixels(key, upper=False)[0], s, pad, y), "hi h", dur)
        svg.rects(pixel_rects(text_pixels(value, upper=False)[0], s, pad + keyw, y), "va h", dur)
    ramp = ("#1c1c1c", "#2e2e2e", "#474747", "#626262", "#7e7e7e", "#9c9c9c", "#c0c0c0", "#e4e4e4")
    dur = f' style="animation-duration:{0.5 + len(lines) * 0.09:.2f}s"'
    for k, col in enumerate(ramp):
        svg.body.append(f'<rect class="h" x="{pad + k * 26}" y="{62 + y0 + len(lines) * pitch + 16}" '
                        f'width="26" height="12" fill="{col}"{dur}/>')
    return svg


def card_svg(title, caption, logo, theme, primary=False, W=380, H=76, gap=8):
    """Link card: pixel logo, title, one-line caption, ↗. `gap` spaces wrapped rows."""
    s, tx = 2, 74
    room = W - tx - 40
    if text_layout(caption, upper=False)[1] * s > room:
        while caption and text_layout(caption + "..", upper=False)[1] * s > room:
            caption = caption[:-1]
        caption = caption.rstrip() + ".."
    svg = Svg(W, H + gap, None, f"{title}: {caption}")
    screen(svg, W, H, theme)
    if primary:  # inner ring marks the recommended option
        svg.rects(frame_rects(4, 4, W - 8, H - 8), "pr")
    svg.rects(pixel_rects(bitmap(LOGOS.get(logo, LOGOS["heart"])), 2, 18, (H - 40) // 2), "hi")
    svg.rects(pixel_rects(text_pixels(title, upper=False)[0], s, tx, 19), "hi")
    svg.rects(pixel_rects(text_pixels(caption, upper=False)[0], s, tx, 41), "va")
    svg.rects(pixel_rects(bitmap(ARROW), 2, W - 32, 19), "hi" if primary else "sb")
    return svg

