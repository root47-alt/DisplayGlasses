# Optics calculator

Models the 0.96" OLED, one convex lens and a beam splitter (glass cube, or flat plate, round or rectangular) and answers the questions that decide what to buy and how big the glasses module gets: where the screen goes to be in focus, how big the text looks, whether your eye sees the whole screen, corners included, and whether the splitter clears your eyelashes.

Two versions with identical math:

- **Web page:** `calculator.html` (with `optics.js` next to it). Open it in any browser. It has sliders, a simulated view through the glasses and a to-scale top view.
- **Command line:** `optics.py` (Python 3, nothing to install).

```bash
python3 tools/optics/optics.py
```

```bash
python3 tools/optics/optics.py --f 60 --lens 30 --shape rect --size 35 --plate-h 25
```

```bash
python3 tools/optics/optics.py --sweep
```

`--shape` picks a round plate (`--size` is its diameter) or a rectangular one (`--size` is the width that tilts, `--plate-h` the height). Without `--eye`, the splitter sits as close to your eye as its near edge allows. `--sweep` prints a table comparing common lenses and splitters.

## How it works

Thin-lens optics for focus and image size. For what the eye can see, it traces rays backward from the pupil (4 mm) through the splitter and the lens, with the fold unfolded into a straight line. It aims at a grid of screen points that includes the corners, because the image's corners pass through the splitter's corners and the edge of the round lens. For a cube it includes refraction inside the glass.

- A screen point counts as visible when its rays get through every edge.
- The **eye box** is the range of eye positions that see the whole screen at full brightness.
- **Half bright** only needs the ray through the middle of the pupil, so an edge or corner can drop to about half brightness.
- The **near edge** is how close the tilted splitter comes to your pupil. Under 16 mm it may touch your eyelashes.

Real lenses blur a little toward the edges and the parts won't match the listings exactly, so check the numbers on the optics bench.
