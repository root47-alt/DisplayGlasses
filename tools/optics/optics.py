#!/usr/bin/env python3
"""Optics calculator for the DisplayGlasses heads-up display.

Models the 0.96" OLED, a single convex lens and a beam splitter (a glass cube,
or a flat plate at 45 degrees, round or rectangular) with thin-lens optics and
ray tracing, and answers the questions that decide what to buy and how big the
glasses module gets:

  * Where does the screen go so the text is in focus? How much room for error?
  * How big does the screen look (field of view), and how big is the text?
  * Can the eye see the whole screen, corners included, and how far can the
    glasses shift on your face before the edges dim (the "eye box")?
  * Does the tilted splitter clear your eyelashes?

Run it:
  python3 tools/optics/optics.py                     # the default setup
  python3 tools/optics/optics.py --f 50 --lens 25
  python3 tools/optics/optics.py --shape rect --size 35 --plate-h 25
  python3 tools/optics/optics.py --splitter cube --size 20
  python3 tools/optics/optics.py --sweep             # compare common parts

All lengths are millimeters, angles are degrees. No dependencies.
"""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass, field

# 0.96" SSD1306, 128 x 64 pixels at 0.17 mm pitch.
SCREEN_W = 21.744
SCREEN_H = 10.864
PIXEL = 0.17
CAP_HEIGHT_PX = 7  # capital letters in the firmware's 6x10 font

GLASS_N = 1.5          # refractive index of a glass cube
GLASS_DENSITY = 2.5    # g/cm^3
PUPIL = 4.0            # indoor pupil diameter
LENS_RIM = 1.0         # the holder covers this much of the lens edge
SQRT2 = math.sqrt(2)

# Eyelashes: the splitter's nearest edge should stay about 16 mm from the pupil
# (about 12.5 mm from the eye's surface). A 25 mm plate centered 25 mm out
# leaves 16.2 mm, so that's the target; closer than 16 mm gets a warning.
NEAR_EDGE_TARGET = 16.2
NEAR_EDGE_MIN = 16.0

# Screen points checked for the eye box, as fractions of the half width and
# half height: corners first (they fail first), then edges, then the middle.
SCREEN_POINTS = ((-1, -1), (1, -1), (-1, 1), (1, 1),
                 (-0.5, -1), (0.5, -1), (-0.5, 1), (0.5, 1),
                 (-1, 0), (1, 0), (-0.5, 0), (0.5, 0),
                 (0, -1), (0, 1), (0, 0))

# Where rays leave the pupil, as fractions of its radius: a ring of 12 at the
# edge, a ring of 12 halfway in, and the center.
PUPIL_POINTS = tuple((k * math.cos(n * math.pi / 6), k * math.sin(n * math.pi / 6))
                     for k in (1.0, 0.5) for n in range(12)) + ((0.0, 0.0),)


@dataclass
class Setup:
    f: float = 60.0                 # lens focal length
    lens_d: float = 30.0            # lens diameter
    splitter: str = "plate"         # "cube" or "plate"
    size: float = 25.0              # cube side; plate diameter (round) or width along its 45 degree tilt (rect)
    plate_shape: str = "round"      # "round" or "rect" (plates only)
    plate_h: float = 25.0           # plate height (rect only; a round plate's height is its diameter)
    plate_t: float = 1.0            # plate thickness (plates only)
    # Pupil to the splitter's center. 25 mm keeps the near edge of a 25 mm
    # plate 16.2 mm from the pupil, clear of eyelashes; see safe_eye_distance().
    eye_to_splitter: float = 25.0
    lens_to_splitter: float | None = None  # lens to splitter center; None = as close as fits
    image_dist: float = 2000.0      # where the text should appear to float, from the eye
    pupil: float = PUPIL

    def __post_init__(self) -> None:
        if self.splitter == "plate" and self.plate_shape == "round":
            self.plate_h = self.size
        if self.lens_to_splitter is None:
            self.lens_to_splitter = self.splitter_depth() / 2 + 4.0

    def splitter_depth(self) -> float:
        """How much room the splitter takes along the light path."""
        return self.size if self.splitter == "cube" else self.size / SQRT2

    def near_offset(self) -> float:
        """How much closer to the eye the splitter's nearest point is than its center."""
        return self.size / 2 if self.splitter == "cube" else self.size / (2 * SQRT2)


def safe_eye_distance(splitter: str, size: float) -> float:
    """Pupil to splitter center that keeps the near edge clear of eyelashes."""
    return round(NEAR_EDGE_TARGET + Setup(splitter=splitter, size=size).near_offset(), 1)


@dataclass
class Result:
    setup: Setup
    lens_to_eye: float          # air-equivalent distance from the lens to the eye
    screen_to_lens: float       # for the chosen image distance
    focus_range: tuple[float, float]  # screen-to-lens for text at 0.5 m .. infinity
    fov_h: float
    fov_v: float
    text_mm_at_1m: float        # capital letter height, as if printed on a sign 1 m away
    eyebox_h: float             # how far the eye can move sideways and still see everything, undimmed
    eyebox_v: float
    eyebox_half_h: float        # same, but an edge may dim to about half brightness
    eyebox_half_v: float
    visible_h: float            # fraction of the screen width visible from the centered eye
    visible_v: float
    near_edge: float            # pupil to the splitter's nearest edge
    path_length: float          # screen to splitter center, along the light path
    splitter_grams: float
    notes: list[str] = field(default_factory=list)


# ------------------------------------------------------------------ tracing

def _glass_slopes(sy: float, sv: float) -> tuple[float, float]:
    """Ray slopes inside glass for a ray with slopes (sy, sv) in air (Snell's law)."""
    k = 1 / (GLASS_N * math.sqrt(1 + sy * sy + sv * sv))
    ty, tv = sy * k, sv * k
    tz = math.sqrt(1 - ty * ty - tv * tv)
    return ty / tz, tv / tz


def _ray_visible(s: Setup, y0: float, v0: float, sy: float, sv: float, lens_z: float) -> bool:
    """Traces one ray backward from the eye toward the screen.

    The eye looks along z; y is sideways (the direction the splitter tilts)
    and v is up/down. The fold at the splitter is unfolded into a straight
    line, so the path runs along z from the pupil (z = 0) to the lens. The ray
    leaves the pupil at (y0, v0) with slopes sy and sv. Returns True if it
    gets through every aperture.
    """
    e = s.eye_to_splitter
    if s.splitter == "cube":
        c = s.size
        g = e - c / 2                       # pupil to the cube's near face
        y, v = y0 + g * sy, v0 + g * sv
        if abs(y) > c / 2 or abs(v) > c / 2:
            return False
        gy, gv = _glass_slopes(sy, sv)
        # Inside the cube, any ray between the faces meets the diagonal mirror.
        y, v = y + c * gy, v + c * gv
        if abs(y) > c / 2 or abs(v) > c / 2:
            return False
        rest = lens_z - (g + c)
        y_lens, v_lens = y + rest * sy, v + rest * sv
    else:
        # Mirror tilted 45 degrees sideways: the plane y = z - e.
        if abs(1 - sy) < 1e-9:
            return False
        z_hit = (y0 + e) / (1 - sy)
        u = (z_hit - e) * SQRT2             # across the plate, along its tilt
        v = v0 + z_hit * sv                 # up/down on the plate
        a, b = s.size / 2, s.plate_h / 2
        if s.plate_shape == "round":
            if (u / a) ** 2 + (v / b) ** 2 > 1:
                return False
        elif abs(u) > a or abs(v) > b:
            return False
        y_lens, v_lens = y0 + lens_z * sy, v0 + lens_z * sv
    r = s.lens_d / 2 - LENS_RIM
    return y_lens * y_lens + v_lens * v_lens <= r * r


def _eyebox(s: Setup, slope_h: float, slope_v: float, horizontal: bool, lens_z: float,
            full_pupil: bool = True) -> float:
    """Width of the range of eye positions that see the whole screen.

    Moves the eye sideways (horizontal=True) or up/down and checks a grid of
    screen points, corners included. full_pupil=True asks that every ray
    through the pupil gets through (no dimming anywhere); False only needs the
    pupil's center ray (an edge at worst about half as bright).
    """
    half = s.pupil / 2
    pupil = [(p * half, q * half) for p, q in PUPIL_POINTS] if full_pupil else [(0.0, 0.0)]
    rays = [(i * slope_h, j * slope_v, py, pv) for i, j in SCREEN_POINTS for py, pv in pupil]
    step = 0.05
    lo = hi = None
    for i in range(801):  # eye positions from -20 to +20 mm
        d = -20.0 + i * step
        y0, v0 = (d, 0.0) if horizontal else (0.0, d)
        if all(_ray_visible(s, y0 + py, v0 + pv, sy, sv, lens_z) for sy, sv, py, pv in rays):
            if lo is None:
                lo = d
            hi = d
    return 0.0 if lo is None else hi - lo + step


def _visible_fraction(s: Setup, slope_h: float, slope_v: float, horizontal: bool, lens_z: float) -> float:
    """Fraction of the screen's width (or height) the centered eye sees.

    A column counts when its top, middle and bottom all show (a row: its left
    end, middle and right end), so cut-off corners count too.
    """
    n = 101
    seen = 0
    for i in range(n):
        k = 2 * i / (n - 1) - 1
        pts = [(k * slope_h, j * slope_v) if horizontal else (j * slope_h, k * slope_v) for j in (-1, 0, 1)]
        if all(_ray_visible(s, 0.0, 0.0, sy, sv, lens_z) for sy, sv in pts):
            seen += 1
    return seen / n


# ---------------------------------------------------------------- analysis

def screen_distance(f: float, image_from_lens: float) -> float:
    """Screen-to-lens distance that puts a virtual image at the given distance."""
    if math.isinf(image_from_lens):
        return f
    return f * image_from_lens / (f + image_from_lens)


def analyze(s: Setup) -> Result:
    # Air-equivalent distance from the eye to the lens (glass makes it look shorter).
    if s.splitter == "cube":
        lens_to_eye = s.eye_to_splitter + s.lens_to_splitter - s.size * (1 - 1 / GLASS_N)
    else:
        lens_to_eye = s.eye_to_splitter + s.lens_to_splitter
    lens_z = s.eye_to_splitter + s.lens_to_splitter  # where the lens is along the unfolded path

    def dist_for(image_from_eye: float) -> float:
        return screen_distance(s.f, image_from_eye - lens_to_eye)

    sd = dist_for(s.image_dist)
    near = dist_for(500.0)
    far = s.f

    # Angular size of the screen at the chosen focus.
    v = sd * s.f / (s.f - sd)          # virtual image distance from the lens
    mag = v / sd
    slope_h = (SCREEN_W / 2) * mag / (v + lens_to_eye)
    slope_v = (SCREEN_H / 2) * mag / (v + lens_to_eye)
    fov_h = 2 * math.degrees(math.atan(slope_h))
    fov_v = 2 * math.degrees(math.atan(slope_v))
    cap = CAP_HEIGHT_PX * PIXEL * mag / (v + lens_to_eye) * 1000.0

    if s.splitter == "cube":
        grams = (s.size / 10) ** 3 * GLASS_DENSITY
    else:
        area = math.pi / 4 * s.size * s.size if s.plate_shape == "round" else s.size * s.plate_h
        grams = area / 100 * (s.plate_t / 10) * GLASS_DENSITY

    r = Result(
        setup=s,
        lens_to_eye=lens_to_eye,
        screen_to_lens=sd,
        focus_range=(near, far),
        fov_h=fov_h,
        fov_v=fov_v,
        text_mm_at_1m=cap,
        eyebox_h=_eyebox(s, slope_h, slope_v, True, lens_z),
        eyebox_v=_eyebox(s, slope_h, slope_v, False, lens_z),
        eyebox_half_h=_eyebox(s, slope_h, slope_v, True, lens_z, full_pupil=False),
        eyebox_half_v=_eyebox(s, slope_h, slope_v, False, lens_z, full_pupil=False),
        visible_h=_visible_fraction(s, slope_h, slope_v, True, lens_z),
        visible_v=_visible_fraction(s, slope_h, slope_v, False, lens_z),
        near_edge=s.eye_to_splitter - s.near_offset(),
        path_length=sd + s.lens_to_splitter,
        splitter_grams=grams,
    )
    if r.eyebox_h < 3 or r.eyebox_v < 3:
        r.notes.append("tight eye box: small shifts of the glasses will dim the edges or corners")
    if r.visible_h < 0.999 or r.visible_v < 0.999:
        r.notes.append("parts of the screen are cut off even with the eye centered")
    if r.near_edge < NEAR_EDGE_MIN:
        r.notes.append(f"the splitter's near edge is only {r.near_edge:.1f} mm from your pupil and may touch your "
                       f"eyelashes; move it {NEAR_EDGE_TARGET - r.near_edge:.1f} mm farther out "
                       f"(eye to splitter {s.eye_to_splitter + NEAR_EDGE_TARGET - r.near_edge:.1f} mm)")
    if r.fov_h > 28:
        r.notes.append("very wide image: it covers a big part of that eye's view")
    if s.splitter == "plate" and s.image_dist < 1000:
        r.notes.append("a plate's back surface makes a faint second image; focus the text 1 m or farther "
                       "and it lines up with the main one")
    return r


def splitter_name(s: Setup) -> str:
    if s.splitter == "cube":
        return f"{s.size:g} mm cube"
    if s.plate_shape == "round":
        return f"{s.size:g} mm round plate"
    return f"{s.size:g} x {s.plate_h:g} mm plate"


def describe(r: Result) -> str:
    s = r.setup
    lines = [
        f"Lens f={s.f:g} mm, {s.lens_d:g} mm wide; {splitter_name(s)}; eye {s.eye_to_splitter:g} mm from the splitter center",
        f"  Screen to lens:     {r.screen_to_lens:5.1f} mm for text {s.image_dist / 1000:g} m away "
        f"(focus range {r.focus_range[0]:.1f}-{r.focus_range[1]:.1f} mm = text at 0.5 m to infinity)",
        f"  Lens to splitter:   {s.lens_to_splitter:5.1f} mm   (light path screen -> splitter: {r.path_length:.0f} mm)",
        f"  Image size:         {r.fov_h:4.1f} x {r.fov_v:4.1f} degrees",
        f"  Text size:          capitals look {r.text_mm_at_1m:.0f} mm tall on a sign 1 m away",
        f"  Eye box:            {r.eyebox_h:4.1f} mm sideways, {r.eyebox_v:4.1f} mm up/down (whole screen, no dimming)",
        f"                      {r.eyebox_half_h:4.1f} mm sideways, {r.eyebox_half_v:4.1f} mm up/down "
        f"(before an edge drops to about half brightness)",
        f"  Visible, eye centered: {r.visible_h * 100:.0f}% of the width, {r.visible_v * 100:.0f}% of the height",
        f"  Near edge:          {r.near_edge:4.1f} mm from your pupil ({NEAR_EDGE_MIN:g} mm or more clears most eyelashes)",
        f"  Splitter weight:    {r.splitter_grams:.1f} g",
    ]
    lines += [f"  Note: {n}" for n in r.notes]
    return "\n".join(lines)


# Splitters for --sweep: (splitter, plate shape, size, plate height).
SWEEP_SPLITTERS = (
    ("plate", "round", 20.0, 20.0),
    ("plate", "round", 25.0, 25.0),
    ("plate", "rect", 25.0, 25.0),
    ("plate", "rect", 30.0, 25.0),
    ("plate", "rect", 35.0, 25.0),
    ("cube", "round", 15.0, 15.0),
    ("cube", "round", 20.0, 20.0),
)


def sweep() -> str:
    """Markdown table comparing common lenses and splitters, each at its eyelash-safe distance."""
    out = ["| Splitter | Eye → splitter | Lens f | Lens width | Image size | Text at 1 m "
           "| Eye box, no dimming | Before an edge is half as bright | Visible | Screen→splitter |",
           "|---|---|---|---|---|---|---|---|---|---|"]
    for splitter, shape, size, h in SWEEP_SPLITTERS:
        eye = safe_eye_distance(splitter, size)
        for f in (50.0, 60.0, 70.0):
            for d in (25.0, 30.0, 35.0):
                s = Setup(f=f, lens_d=d, splitter=splitter, plate_shape=shape, size=size, plate_h=h,
                          eye_to_splitter=eye)
                r = analyze(s)
                vis = min(r.visible_h, r.visible_v)
                out.append(
                    f"| {splitter_name(s)} | {eye:g} mm | {f:g} mm | {d:g} mm | {r.fov_h:.0f}° × {r.fov_v:.0f}° "
                    f"| {r.text_mm_at_1m:.0f} mm | {r.eyebox_h:.1f} × {r.eyebox_v:.1f} mm "
                    f"| {r.eyebox_half_h:.1f} × {r.eyebox_half_v:.1f} mm | {vis * 100:.0f}% | {r.path_length:.0f} mm |")
    return "\n".join(out)


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--f", type=float, default=60.0, help="lens focal length (mm)")
    p.add_argument("--lens", type=float, default=30.0, help="lens diameter (mm)")
    p.add_argument("--splitter", choices=["cube", "plate"], default="plate")
    p.add_argument("--shape", choices=["round", "rect"], default="round", help="plate shape")
    p.add_argument("--size", type=float, default=None,
                   help="cube side, plate diameter (round) or plate width along its tilt (rect) (mm)")
    p.add_argument("--plate-h", type=float, default=25.0, help="rectangular plate height (mm)")
    p.add_argument("--eye", type=float, default=None,
                   help="pupil to splitter center (mm); default: as close as clears your eyelashes")
    p.add_argument("--lens-gap", type=float, default=None, help="lens to splitter center (mm)")
    p.add_argument("--image", type=float, default=2.0, help="where the text should float (meters)")
    p.add_argument("--sweep", action="store_true", help="print a comparison table instead")
    a = p.parse_args()
    if a.sweep:
        print(sweep())
        return
    size = a.size if a.size is not None else (15.0 if a.splitter == "cube" else 25.0)
    eye = a.eye if a.eye is not None else safe_eye_distance(a.splitter, size)
    s = Setup(f=a.f, lens_d=a.lens, splitter=a.splitter, plate_shape=a.shape, size=size, plate_h=a.plate_h,
              eye_to_splitter=eye, lens_to_splitter=a.lens_gap, image_dist=a.image * 1000)
    print(describe(analyze(s)))


if __name__ == "__main__":
    main()
