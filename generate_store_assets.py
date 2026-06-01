"""
Generates Pebble store assets for the PNOOKA watchface.
All assets are rendered programmatically — no device screenshots required.

Outputs:
  store_assets/screenshot_dark_3x.png    432×504  (mint-on-dark theme)
  store_assets/screenshot_classic_3x.png 432×504  (dark-on-light theme)
  store_assets/icon_48.png               48×48
  store_assets/banner_720x320.png        720×320
"""

from PIL import Image, ImageDraw, ImageFont
import os

OUT = os.path.join(os.path.dirname(__file__), "store_assets")
os.makedirs(OUT, exist_ok=True)

# ── Palette ──────────────────────────────────────────────────────────────────
BG_DARK    = (18, 18, 18)
BG_LIGHT   = (245, 245, 245)
MINT       = (109, 213, 174)
DARK_DOT   = (80, 80, 80)
LIGHT_DOT  = (160, 160, 160)
ACCENT     = MINT


def load_font(size):
    for path in [
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/SFNSDisplay.ttf",
    ]:
        try:
            return ImageFont.truetype(path, size)
        except Exception:
            pass
    return ImageFont.load_default()


# ── Watchface renderer ───────────────────────────────────────────────────────
def draw_watchface(hours, minutes, is_pm, date_str, bg, filled, empty):
    """
    Renders PNOOKA at 144×168 (Pebble Time resolution).
    Layout constants mirror main.c exactly.
    Scale 3× with NEAREST for store screenshots.
    """
    PW, PH = 144, 168
    img = Image.new("RGBA", (PW, PH), bg)
    d   = ImageDraw.Draw(img)

    GRID_ROWS, GRID_COLS = 3, 4
    radius  = 8
    spacing = 32
    bar_h   = 14
    bar_gap = spacing - 2 * radius   # = 16
    font_h  = 18

    grid_w = (GRID_COLS - 1) * spacing + radius * 2   # 112
    margin = (PW - grid_w) // 2 - 1                    # 15
    grid_x = margin + radius                            # 23

    total_h = (GRID_ROWS-1)*spacing + 2*radius + bar_gap + bar_h + bar_gap//2 + font_h
    grid_y  = (PH - total_h) // 2 + radius + 2

    # ── Dot grid (column-major fill) ──────────────────────────────────────────
    for col in range(GRID_COLS):
        for row in range(GRID_ROWS):
            hour     = col * GRID_ROWS + row + 1
            cx       = grid_x + col * spacing
            cy       = grid_y + row * spacing
            dot_fill = filled if hour <= hours else empty
            d.ellipse([cx-radius, cy-radius, cx+radius, cy+radius], fill=dot_fill)

    # ── Segmented minutes bar (1px segments, 1px gap each) ───────────────────
    seg_sp = 2
    bar_w  = 60 * seg_sp   # 120
    bm     = (PW - bar_w) // 2 - 1
    bar_y  = grid_y + (GRID_ROWS-1)*spacing + radius + bar_gap

    for i in range(60):
        seg_fill = filled if i < minutes else empty
        sx = bm + i * seg_sp
        d.rectangle([sx, bar_y, sx, bar_y + bar_h - 1], fill=seg_fill)

    for i in range(0, 61, 10):
        pos = 59 if i == 60 else i
        sx  = bm + pos * seg_sp
        d.rectangle([sx, bar_y + bar_h + 1, sx, bar_y + bar_h + 2], fill=filled)

    # ── Bottom text row ───────────────────────────────────────────────────────
    bar_bottom = bar_y + bar_h
    text_y     = bar_bottom + bar_gap // 2 + 2

    r2, g2, b2 = bg[0]//85, bg[1]//85, bg[2]//85
    text_col   = (0, 0, 0) if r2*30 + g2*59 + b2*11 >= 150 else (255, 255, 255)

    fn = load_font(9)

    # Date — left-aligned with bar
    d.text((bm, text_y + 3), date_str, font=fn, fill=text_col)

    # AM/PM indicator — right-aligned with bar
    dot_r   = 3
    lbl_gap = 2
    grp_gap = 4
    x_right = bm + bar_w
    dot_cy  = text_y + font_h // 2

    am_w = int(d.textlength("AM", font=fn))
    pm_w = int(d.textlength("PM", font=fn))

    pm_dot_x = x_right - dot_r
    pm_lx    = pm_dot_x - dot_r - lbl_gap - pm_w
    am_dot_x = pm_lx - grp_gap - dot_r
    am_lx    = am_dot_x - dot_r - lbl_gap - am_w

    d.text((am_lx, text_y + 3), "AM", font=fn, fill=text_col)
    d.ellipse([am_dot_x-dot_r, dot_cy-dot_r, am_dot_x+dot_r, dot_cy+dot_r],
              fill=(empty if is_pm else filled))

    d.text((pm_lx, text_y + 3), "PM", font=fn, fill=text_col)
    d.ellipse([pm_dot_x-dot_r, dot_cy-dot_r, pm_dot_x+dot_r, dot_cy+dot_r],
              fill=(filled if is_pm else empty))

    return img


# ── 1. Screenshots ────────────────────────────────────────────────────────────
SCREENSHOTS = [
    # filename, hours, min, pm, date,       bg,       filled,   empty
    ("screenshot_dark_3x.png",     3, 45, True,  "05/31/26", BG_DARK,  MINT,     DARK_DOT),
    ("screenshot_classic_3x.png",  8, 20, False, "05/31/26", BG_LIGHT, DARK_DOT, LIGHT_DOT),
]

for name, h, m, pm, date, bg, filled, empty in SCREENSHOTS:
    img  = draw_watchface(h, m, pm, date, bg, filled, empty)
    out  = img.resize((img.width * 3, img.height * 3), Image.NEAREST)
    out.save(os.path.join(OUT, name))
    print(f"  saved {name}  ({out.width}×{out.height})")


# ── 2. Icon 48×48 ─────────────────────────────────────────────────────────────
def draw_icon(size=48):
    img = Image.new("RGBA", (size, size), BG_DARK)
    d   = ImageDraw.Draw(img)

    GRID_ROWS, GRID_COLS = 3, 4
    pad     = size * 0.10
    cell_w  = (size - 2 * pad) / GRID_COLS
    cell_h  = (size * 0.72 - pad) / GRID_ROWS   # leave room for bar
    r       = cell_w * 0.28
    active  = 8   # show 8 o'clock (2 full columns + 2 dots)

    for col in range(GRID_COLS):
        for row in range(GRID_ROWS):
            hour = col * GRID_ROWS + row + 1
            cx   = pad + col * cell_w + cell_w / 2
            cy   = pad + row * cell_h + cell_h / 2
            colour = MINT if hour <= active else DARK_DOT
            d.ellipse([cx-r, cy-r, cx+r, cy+r], fill=colour)

    # Segmented bar (~60% full)
    bar_h  = max(2, int(size * 0.06))
    bar_y  = size - pad - bar_h
    bar_x0 = pad
    bar_x1 = size - pad
    filled_x = bar_x0 + (bar_x1 - bar_x0) * 0.60

    # Draw tiny segments
    total_w = bar_x1 - bar_x0
    for i in range(30):   # simplified: 30 ticks across icon width
        x = bar_x0 + i * total_w / 30
        colour = MINT if x < filled_x else DARK_DOT
        d.rectangle([x, bar_y, x + total_w/30 - 1, bar_y + bar_h], fill=colour)

    return img

icon = draw_icon(48)
icon.save(os.path.join(OUT, "icon_48.png"))
print("  saved icon_48.png  (48×48)")


# ── 3. Banner 720×320 ─────────────────────────────────────────────────────────
BW, BH = 720, 320
banner = Image.new("RGBA", (BW, BH), BG_DARK)
d      = ImageDraw.Draw(banner)

# Decorative background dots
for gx in range(0, BW, 30):
    for gy in range(0, BH, 30):
        d.ellipse([gx-1, gy-1, gx+1, gy+1], fill=(35, 35, 35))

# Title
font_title = load_font(64)
font_sub   = load_font(20)
font_tiny  = load_font(15)

title_x = 48
d.text((title_x, 72), "PNOOKA", font=font_title, fill=(255, 255, 255))
bbox   = d.textbbox((title_x, 72), "PNOOKA", font=font_title)
line_y = bbox[3] + 6
d.rectangle([title_x, line_y, title_x + 140, line_y + 3], fill=ACCENT)
d.text((title_x, line_y + 14), "the P is silent.", font=font_sub, fill=(160, 160, 160))
d.text((title_x, line_y + 44), "Minimal dot-matrix watchface\nfor Pebble Time",
       font=font_tiny, fill=(110, 110, 110))

# Embed watchface screenshots on the right
target_h = 200
gap      = 20
screens  = [
    draw_watchface(3, 45, True,  "05/31/26", BG_DARK,  MINT,     DARK_DOT),
    draw_watchface(8, 20, False, "05/31/26", BG_LIGHT, DARK_DOT, LIGHT_DOT),
]
labels = ["Mint", "Classic"]

scale_factor = target_h / screens[0].height
sw = int(screens[0].width  * scale_factor)
sh = int(screens[0].height * scale_factor)

total_w = sw * 2 + gap
start_x = BW - total_w - 40
start_y = (BH - sh) // 2

for i, (ss, label) in enumerate(zip(screens, labels)):
    ss_scaled = ss.resize((sw, sh), Image.NEAREST)
    ox = start_x + i * (sw + gap)
    border = 2
    d.rectangle([ox-border, start_y-border, ox+sw+border, start_y+sh+border],
                outline=ACCENT, width=border)
    banner.paste(ss_scaled, (ox, start_y), ss_scaled)
    d.text((ox, start_y + sh + 10), label, font=font_tiny, fill=(100, 100, 100))

banner.save(os.path.join(OUT, "banner_720x320.png"))
print("  saved banner_720x320.png  (720×320)")

print("\nAll store assets saved to ./store_assets/")
print("Files:")
for f in sorted(os.listdir(OUT)):
    path = os.path.join(OUT, f)
    sz   = os.path.getsize(path)
    img  = Image.open(path)
    print(f"  {f:38s}  {img.size[0]}×{img.size[1]}  ({sz//1024} KB)")
