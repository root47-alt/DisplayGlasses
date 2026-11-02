#!/usr/bin/env python3
"""Optics calculator for the DisplayGlasses heads-up display.

Models the 0.96" OLED, a single convex lens and a beam splitter (a glass cube
or a flat plate at 45 degrees) with thin-lens optics and simple ray tracing,
and answers the questions that decide what to buy and how big the glasses
module gets:

  * Where does the screen go so the text is in focus? How much room for error?
  * How big does the screen look (field of view), and how big is the text?
  * Can the eye see the whole screen, and how far can the glasses shift on
    your face before the edges get cut off (the "eye box")?

Run it:
  python3 tools/optics/optics.py                     # the default setup
  python3 tools/optics/optics.py --f 50 --lens 25 --splitter plate
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


@dataclass
class Setup:
    f: float = 50.0                 # lens focal length
    lens_d: float = 25.0            # lens diameter
    splitter: str = "plate"         # "cube" or "plate"
    size: float = 25.0              # cube side, or plate width along its 45 degree tilt
    plate_h: float = 20.0           # plate height (plates only)
    plate_t: float = 1.0            # plate thickness (plates only)
    eye_to_splitter: float = 20.0   # eye to the splitter's center
    lens_to_splitter: float | None = None  # lens to splitter center; None = as close as fits
    image_dist: float = 2000.0      # where the text should appear to float, from the eye
    pupil: float = PUPIL

    def __post_init__(self) -> None:
        if self.lens_to_splitter is None:
            self.lens_to_splitter = self.splitter_depth() / 2 + 4.0

    def splitter_depth(self) -> float:
        """How much room the splitter takes along the light path."""
        return self.size if self.splitter == "cube" else self.size / math.sqrt(2)


@dataclass
class Result:
    setup: Setup
    lens_to_eye: float          # air-equivalent distance from the lens to the eye
    screen_to_lens: float       # for the chosen image distance
    focus_range: tuple[float, float]  # screen-to-lens for text at 0.5 m .. infinity
    fov_h: float
    fov_v: float
    text_mm_at_1m: float        # capital letter height, as if printed on a sign 1 m away
    eyebox_h: float             # how far the eye can move sideways and still see everything
    eyebox_v: float
    visible_h: float            # fraction of the screen width visible from the centered eye
    visible_v: float
    path_length: float          # screen to splitter center, along the light path
    splitter_grams: float
    notes: list[str] = field(default_factory=list)


# ------------------------------------------------------------------ tracing

def _glass_slope(slope: float) -> float:
    """Ray slope inside glass for a ray with the given slope in air."""
    angle = math.atan(slope)
    return math.tan(math.asin(math.sin(angle) / GLASS_N))


def _ray_visible(s: Setup, y0: float, slope: float, horizontal: bool, lens_z: float) -> bool:
    """Traces one ray backward from the eye (offset y0) toward the screen.

    The fold at the splitter is unfolded into a straight line, so the light
    path runs along z from the eye (z = 0) to the lens. Returns True if the
    ray gets through every aperture.
    """
    r_lens = s.lens_d / 2 - LENS_RIM
    e = s.eye_to_splitter
    if s.splitter == "cube":
        c = s.size
        g = e - c / 2                       # eye to the cube's near face
        y_near = y0 + g * slope
        y_far = y_near + c * _glass_slope(slope)
        if abs(y_near) > c / 2 or abs(y_far) > c / 2:
            return False
        # Inside the cube, any ray between the faces meets the diagonal mirror.
        y_lens = y_far + (lens_z - (g + c)) * slope
    else:
        if horizontal:
            # Mirror tilted 45 degrees in this plane: the line y = z - e.
            # Light comes in from the screen's side; the far edge limits it.
            if abs(1 - slope) < 1e-9:
                return False
            z_hit = (y0 + e) / (1 - slope)
            y_hit = z_hit - e
            if abs(y_hit) > s.size / (2 * math.sqrt(2)):
                return False
        else:
            if abs(y0 + e * slope) > s.plate_h / 2:
                return False
        y_lens = y0 + lens_z * slope
    return abs(y_lens) <= r_lens


def _field_visible(s: Setup, eye_offset: float, slope: float, horizontal: bool, lens_z: float,
                   full_pupil: bool) -> bool:
    """Is a screen point (seen at `slope`) visible from an eye at `eye_offset`?

    full_pupil=True asks that the whole pupil sees it (no dimming);
    False only needs the pupil's center ray.
    """
    if not full_pupil:
        return _ray_visible(s, eye_offset, slope, horizontal, lens_z)
    half = s.pupil / 2
    return all(_ray_visible(s, eye_offset + u * half, slope, horizontal, lens_z) for u in (-1, -0.5, 0, 0.5, 1))


def _eyebox(s: Setup, max_slope: float, horizontal: bool, lens_z: float) -> float:
    """Width of the range of eye positions that see the whole screen undimmed."""
    step = 0.05
    ok = []
    for i in range(801):  # eye positions from -20 to +20 mm
        y = -20.0 + i * step
        if all(_field_visible(s, y, k * max_slope, horizontal, lens_z, True) for k in (-1, -0.5, 0, 0.5, 1)):
            ok.append(y)
    return (max(ok) - min(ok) + step) if ok else 0.0


def _visible_fraction(s: Setup, max_slope: float, horizontal: bool, lens_z: float) -> float:
    """Fraction of the screen visible (center ray) with the eye centered."""
    n = 101
    seen = sum(_field_visible(s, 0.0, (2 * i / (n - 1) - 1) * max_slope, horizontal, lens_z, False)
               for i in range(n))
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
        grams = (s.size / 10) * (s.plate_h / 10) * (s.plate_t / 10) * GLASS_DENSITY

    r = Result(
        setup=s,
        lens_to_eye=lens_to_eye,
        screen_to_lens=sd,
        focus_range=(near, far),
        fov_h=fov_h,
        fov_v=fov_v,
        text_mm_at_1m=cap,
        eyebox_h=_eyebox(s, slope_h, True, lens_z),
        eyebox_v=_eyebox(s, slope_v, False, lens_z),
        visible_h=_visible_fraction(s, slope_h, True, lens_z),
        visible_v=_visible_fraction(s, slope_v, False, lens_z),
        path_length=sd + s.lens_to_splitter,
        splitter_grams=grams,
    )
    if r.eyebox_h < 3 or r.eyebox_v < 3:
        r.notes.append("tight eye box: small shifts of the glasses will cut off or dim the edges")
    if r.visible_h < 0.999 or r.visible_v < 0.999:
        r.notes.append("the edges of the screen are cut off even with the eye centered")
    if r.fov_h > 28:
        r.notes.append("very wide image: it covers a big part of that eye's view")
    if s.splitter == "plate" and s.image_dist < 1000:
        r.notes.append("a plate's back surface makes a faint second image; focus the text 1 m or farther "
                       "and it lines up with the main one")
    return r


def describe(r: Result) -> str:
    s = r.setup
    kind = f"{s.size:g} mm cube" if s.splitter == "cube" else f"{s.size:g} x {s.plate_h:g} mm plate"
    lines = [
        f"Lens f={s.f:g} mm, {s.lens_d:g} mm wide; {kind}; eye {s.eye_to_splitter:g} mm from the splitter center",
        f"  Screen to lens:     {r.screen_to_lens:5.1f} mm for text {s.image_dist / 1000:g} m away "
        f"(focus range {r.focus_range[0]:.1f}-{r.focus_range[1]:.1f} mm = text at 0.5 m to infinity)",
        f"  Lens to splitter:   {s.lens_to_splitter:5.1f} mm   (light path screen -> splitter: {r.path_length:.0f} mm)",
        f"  Image size:         {r.fov_h:4.1f} x {r.fov_v:4.1f} degrees",
        f"  Text size:          capitals look {r.text_mm_at_1m:.0f} mm tall on a sign 1 m away",
        f"  Eye box:            {r.eyebox_h:4.1f} mm sideways, {r.eyebox_v:4.1f} mm up/down (whole screen, undimmed)",
        f"  Visible, eye centered: {r.visible_h * 100:.0f}% of the width, {r.visible_v * 100:.0f}% of the height",
        f"  Splitter weight:    {r.splitter_grams:.1f} g",
    ]
    lines += [f"  Note: {n}" for n in r.notes]
    return "\n".join(lines)


def sweep() -> str:
    """Markdown table comparing common lenses and splitters."""
    rows = []
    for splitter, size in (("cube", 15.0), ("cube", 20.0), ("plate", 25.0)):
        for f in (30.0, 40.0, 50.0, 60.0):
            for d in (20.0, 25.0, 30.0):
                r = analyze(Setup(f=f, lens_d=d, splitter=splitter, size=size))
                kind = f"{size:g} mm cube" if splitter == "cube" else f"{size:g}x20 mm plate"
                rows.append((kind, f, d, r))
    out = ["| Splitter | Lens f | Lens width | Image size | Text at 1 m | Eye box (side × up) | Visible | Screen→splitter |",
           "|---|---|---|---|---|---|---|---|"]
    for kind, f, d, r in rows:
        vis = min(r.visible_h, r.visible_v)
        out.append(
            f"| {kind} | {f:g} mm | {d:g} mm | {r.fov_h:.0f}° × {r.fov_v:.0f}° | {r.text_mm_at_1m:.0f} mm "
            f"| {r.eyebox_h:.1f} × {r.eyebox_v:.1f} mm | {vis * 100:.0f}% | {r.path_length:.0f} mm |")
    return "\n".join(out)


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--f", type=float, default=50.0, help="lens focal length (mm)")
    p.add_argument("--lens", type=float, default=25.0, help="lens diameter (mm)")
    p.add_argument("--splitter", choices=["cube", "plate"], default="plate")
    p.add_argument("--size", type=float, default=None, help="cube side, or plate width (mm)")
    p.add_argument("--plate-h", type=float, default=20.0, help="plate height (mm)")
    p.add_argument("--eye", type=float, default=20.0, help="eye to splitter center (mm)")
    p.add_argument("--lens-gap", type=float, default=None, help="lens to splitter center (mm)")
    p.add_argument("--image", type=float, default=2.0, help="where the text should float (meters)")
    p.add_argument("--sweep", action="store_true", help="print a comparison table instead")
    a = p.parse_args()
    if a.sweep:
        print(sweep())
        return
    size = a.size if a.size is not None else (15.0 if a.splitter == "cube" else 25.0)
    s = Setup(f=a.f, lens_d=a.lens, splitter=a.splitter, size=size, plate_h=a.plate_h,
              eye_to_splitter=a.eye, lens_to_splitter=a.lens_gap, image_dist=a.image * 1000)
    print(describe(analyze(s)))


if __name__ == "__main__":
    main()
