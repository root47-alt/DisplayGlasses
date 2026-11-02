# Optics calculator

Models the 0.96" OLED, one convex lens and a beam splitter (glass cube or flat plate) and answers the questions that decide what to buy and how big the glasses module gets: where the screen goes to be in focus, how big the text looks, and whether your eye sees the whole screen.

Two versions with identical math:

- **Web page:** `calculator.html` (with `optics.js` next to it). Open it in any browser. It has sliders, a simulated view through the glasses and a to-scale top view.
- **Command line:** `optics.py` (Python 3, nothing to install).

```bash
python3 tools/optics/optics.py
```

```bash
python3 tools/optics/optics.py --f 60 --lens 30 --splitter plate --size 25
```

```bash
python3 tools/optics/optics.py --sweep
```

`--sweep` prints a table comparing common lenses and splitters.

## How it works

Thin-lens optics for focus and image size. For what the eye can see, it traces rays backward from the pupil (4 mm) through the splitter and the lens with the fold unfolded into a straight line. For a cube it includes refraction inside the glass. A screen point counts as visible when its rays get through every edge. The eye box is the range of eye positions that see the whole screen at full brightness.

Real lenses blur a little toward the edges and the parts won't match the listings exactly, so check the numbers on the optics bench.
