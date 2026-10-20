"""README art: the flow diagram and the social preview.

Text is shaped with HarfBuzz and converted to outlines, so GitHub shows exactly
what was rendered here, whatever fonts the viewer has. Every picture comes in a
light and a dark variant for <picture> with prefers-color-scheme.

Needs: python -m pip install fonttools uharfbuzz
Fonts (Inter, JetBrains Mono, both OFL) are downloaded once, checked against a
pinned SHA-256 and cached in ~/.cache/readme-art.
"""

from __future__ import annotations

import hashlib
import io
import os
import re
import urllib.request
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from xml.sax.saxutils import escape

import uharfbuzz as hb
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont

HERE = Path(__file__).resolve().parent
CACHE = Path(os.environ.get("XDG_CACHE_HOME", Path.home() / ".cache")) / "readme-art"

FONT_ZIPS = {
    "inter": ("https://github.com/rsms/inter/releases/download/v4.1/Inter-4.1.zip",
              "9883fdd4a49d4fb66bd8177ba6625ef9a64aa45899767dde3d36aa425756b11e"),
    "mono": ("https://github.com/JetBrains/JetBrainsMono/releases/download/v2.304/JetBrainsMono-2.304.zip",
             "6f6376c6ed2960ea8a963cd7387ec9d76e3f629125bc33d1fdcd7eb7012f7bbf"),
}
FONT_FILES = {
    "regular": ("inter", "extras/ttf/Inter-Regular.ttf"),
    "medium": ("inter", "extras/ttf/Inter-Medium.ttf"),
    "semibold": ("inter", "extras/ttf/Inter-SemiBold.ttf"),
    "bold": ("inter", "extras/ttf/Inter-Bold.ttf"),
    "mono": ("mono", "fonts/ttf/JetBrainsMono-Regular.ttf"),
}


def _fetch_zip(key: str) -> Path:
    url, sha = FONT_ZIPS[key]
    path = CACHE / Path(url).name
    if not path.exists():
        CACHE.mkdir(parents=True, exist_ok=True)
        print(f"downloading {url}")
        with urllib.request.urlopen(url, timeout=60) as r:
            data = r.read()
        if hashlib.sha256(data).hexdigest() != sha:
            raise SystemExit(f"checksum mismatch for {url}")
        path.write_bytes(data)
    return path


class Font:
    def __init__(self, data: bytes):
        self.tt = TTFont(io.BytesIO(data))
        self.glyphs = self.tt.getGlyphSet()
        self.order = self.tt.getGlyphOrder()
        self.upem = self.tt["head"].unitsPerEm
        self.hb = hb.Font(hb.Face(data))
        self.cache: dict[str, str] = {}

    def shape(self, text: str):
        buf = hb.Buffer()
        buf.add_str(text)
        buf.guess_segment_properties()
        hb.shape(self.hb, buf, {"kern": True, "liga": True, "calt": True})
        return buf.glyph_infos, buf.glyph_positions

    def width(self, text: str, size: float) -> float:
        _, pos = self.shape(text)
        return sum(p.x_advance for p in pos) * size / self.upem

    def path(self, text: str, size: float, x: float, y: float) -> str:
        """SVG path data for text with its baseline at y, starting at x."""
        infos, pos = self.shape(text)
        scale = size / self.upem
        out, pen_x = [], 0
        for info, p in zip(infos, pos):
            name = self.order[info.codepoint]
            pen = SVGPathPen(self.glyphs)
            tx = x + (pen_x + p.x_offset) * scale
            ty = y - p.y_offset * scale
            self.glyphs[name].draw(TransformPen(pen, (scale, 0, 0, -scale, tx, ty)))
            d = pen.getCommands()
            if d:
                out.append(d)
            pen_x += p.x_advance
        return re.sub(r"(\d+\.\d{2})\d+", r"\1", " ".join(out))


_FONTS: dict[str, Font] = {}


def font(name: str) -> Font:
    if name not in _FONTS:
        key, member = FONT_FILES[name]
        with zipfile.ZipFile(_fetch_zip(key)) as z:
            _FONTS[name] = Font(z.read(member))
    return _FONTS[name]


# ---- look -------------------------------------------------------------------------------

@dataclass
class Theme:
    name: str
    card: str
    border: str
    title: str
    text: str
    muted: str
    accent: str
    tint: str          # highlighted node
    line: str          # diagram connectors


def themes(accent_light: str, accent_dark: str) -> dict[str, Theme]:
    return {
        "light": Theme("light", "#ffffff", "#d1d9e0", "#1f2328", "#4b535d", "#6e7781",
                       accent_light, accent_light + "1a", "#afb8c1"),
        "dark": Theme("dark", "#151b23", "#3d444d", "#f0f6fc", "#b7bdc8", "#8b949e",
                      accent_dark, accent_dark + "24", "#525964"),
    }


def svg(w: float, h: float, body: str, title: str = "") -> str:
    t = f"<title>{escape(title)}</title>" if title else ""
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w:g}" height="{h:g}" '
            f'viewBox="0 0 {w:g} {h:g}" role="img">{t}{body}</svg>\n')


def text(s: str, x: float, y: float, size: float, fill: str, weight: str = "regular",
         anchor: str = "start", fit: float = 0) -> str:
    """Text as an outline; with fit, shrinks the size until it is at most that wide."""
    f = font(weight)
    if fit:
        while size > 8 and f.width(s, size) > fit:
            size -= 0.25
    if anchor != "start":
        w = f.width(s, size)
        x -= w / 2 if anchor == "middle" else w
    return f'<path fill="{fill}" d="{f.path(s, size, x, y)}"/>'


def wrap(s: str, size: float, width: float, weight: str = "regular") -> list[str]:
    f, lines, cur = font(weight), [], ""
    for word in s.split():
        cand = f"{cur} {word}".strip()
        if cur and f.width(cand, size) > width:
            lines.append(cur)
            cur = word
        else:
            cur = cand
    if cur:
        lines.append(cur)
    return lines


# ---- pictures ---------------------------------------------------------------------------

def header(logo_svg: str, name: str, tagline: str, t: Theme, width: int = 860) -> str:
    """Logo, name and one line under it, centred."""
    logo = 88
    nf, ns = font("bold"), 46
    tw = max(nf.width(name, ns), font("regular").width(tagline, 18))
    x0 = (width - (logo + 24 + tw)) / 2
    inner = re.sub(r"^<svg[^>]*>|</svg>\s*$", "", logo_svg.strip())
    ids = re.findall(r'id="([^"]+)"', inner)
    for i in ids:  # light and dark copies must not share gradient ids
        inner = inner.replace(f'id="{i}"', f'id="{i}-{t.name}"').replace(f"#{i})", f"#{i}-{t.name})")
    body = (f'<svg x="{x0:g}" y="20" width="{logo}" height="{logo}" viewBox="0 0 128 128">{inner}</svg>'
            + text(name, x0 + logo + 24, 72, ns, t.title, "bold")
            + text(tagline, x0 + logo + 26, 102, 18, t.muted))
    return svg(width, 128, body, f"{name}: {tagline}")


@dataclass
class Node:
    title: str
    sub: str = ""
    accent: bool = False


@dataclass
class Flow:
    columns: list[list[Node]]
    width: int = 860
    node_h: int = 58
    gap: int = 14
    labels: list[str] = field(default_factory=list)   # text on the arrows between columns


def flow(f: Flow, t: Theme, alt: str) -> str:
    cols = len(f.columns)
    arrow = 46
    col_w = (f.width - 2 - arrow * (cols - 1)) / cols
    rows = max(len(c) for c in f.columns)
    total_h = rows * f.node_h + (rows - 1) * f.gap
    pad = 2
    h = total_h + 2 * pad
    body, centers = [], []
    for ci, col in enumerate(f.columns):
        x = 1 + ci * (col_w + arrow)
        ch = len(col) * f.node_h + (len(col) - 1) * f.gap
        y0 = pad + (total_h - ch) / 2
        cc = []
        for ni, n in enumerate(col):
            y = y0 + ni * (f.node_h + f.gap)
            stroke = t.accent if n.accent else t.border
            fill = t.tint if n.accent else t.card
            body.append(f'<rect x="{x:g}" y="{y:g}" width="{col_w:g}" height="{f.node_h}" rx="10" '
                        f'fill="{t.card}" stroke="{stroke}"/>')
            if n.accent:
                body.append(f'<rect x="{x:g}" y="{y:g}" width="{col_w:g}" height="{f.node_h}" rx="10" fill="{fill}"/>')
            cx = x + col_w / 2
            if n.sub:
                body.append(text(n.title, cx, y + 25, 14.5, t.title, "semibold", "middle", col_w - 20))
                body.append(text(n.sub, cx, y + 44, 12, t.muted, "mono", "middle", col_w - 20))
            else:
                body.append(text(n.title, cx, y + f.node_h / 2 + 5, 14.5, t.title, "semibold", "middle", col_w - 20))
            cc.append((x, y + f.node_h / 2))
        centers.append(cc)
    for ci in range(cols - 1):
        x1 = centers[ci][0][0] + col_w
        x2 = centers[ci + 1][0][0]
        for (_, y1) in centers[ci]:
            for (_, y2) in centers[ci + 1]:
                if len(centers[ci]) > 1 and len(centers[ci + 1]) > 1 and y1 != y2:
                    continue
                mx = (x1 + x2) / 2
                body.append(f'<path d="M{x1 + 4:g} {y1:g} C{mx:g} {y1:g} {mx:g} {y2:g} {x2 - 8:g} {y2:g}" '
                            f'fill="none" stroke="{t.line}" stroke-width="1.5"/>')
                body.append(f'<path d="M{x2 - 9:g} {y2 - 4:g} L{x2 - 3:g} {y2:g} L{x2 - 9:g} {y2 + 4:g}" '
                            f'fill="none" stroke="{t.line}" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>')
    return svg(f.width, h, "".join(body), alt)


def social(logo_svg: str, name: str, tagline: str, accent: str, out_png: Path) -> None:
    """1280x640 social preview (GitHub → Settings → Social preview)."""
    import subprocess
    import tempfile
    t = themes(accent, accent)["light"]
    inner = re.sub(r"^<svg[^>]*>|</svg>\s*$", "", logo_svg.strip())
    lines = wrap(tagline, 34, 980)
    body = [f'<rect width="1280" height="640" fill="#ffffff"/>',
            f'<rect width="1280" height="10" fill="{accent}"/>',
            f'<svg x="150" y="{190 - 24 * (len(lines) - 1)}" width="170" height="170" viewBox="0 0 128 128">{inner}</svg>',
            text(name, 352, 300 - 24 * (len(lines) - 1), 92, t.title, "bold")]
    for i, line in enumerate(lines):
        body.append(text(line, 154, 430 - 24 * (len(lines) - 1) + 48 * i, 34, t.text))
    body.append(text("github.com/Trendorin/" + name.lower(), 154, 560, 24, t.muted, "mono"))
    with tempfile.NamedTemporaryFile("w", suffix=".svg", delete=False) as fh:
        fh.write(svg(1280, 640, "".join(body)))
    subprocess.run(["rsvg-convert", "-w", "1280", "-h", "640", fh.name, "-o", str(out_png)], check=True)
    os.unlink(fh.name)


def write(out: Path, name: str, variants: dict[str, str]) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for theme, content in variants.items():
        (out / f"{name}-{theme}.svg").write_text(content)


def build(root: Path, name: str, accents: tuple[str, str], tagline: dict,
          flows: dict, flow_alt: dict, app_icons: list[str]) -> None:
    """Everything a README needs, in English and Russian, into root/.github/assets."""
    import shutil
    import subprocess
    out = root / ".github" / "assets"
    logo_path = HERE / "logo.svg"
    logo = logo_path.read_text()
    th = themes(*accents)
    out.mkdir(parents=True, exist_ok=True)
    for f in out.glob("*.svg"):
        f.unlink()
    for lang, sfx in (("en", ""), ("ru", "-ru")):
        write(out, f"flow{sfx}", {k: flow(flows[lang], t, flow_alt[lang]) for k, t in th.items()})
    shutil.copy(logo_path, out / "logo.svg")
    subprocess.run(["rsvg-convert", "-w", "512", "-h", "512", str(logo_path), "-o", str(out / "logo.png")], check=True)
    for p in app_icons:
        shutil.copy(logo_path, root / p)
    social(logo, name, tagline["en"], accents[0], out / "social-preview.png")
    print(f"{name}: {len(list(out.iterdir()))} files in {out.relative_to(root)}")
