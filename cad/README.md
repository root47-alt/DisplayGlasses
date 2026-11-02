# 3D-printed parts

Models are written in [OpenSCAD](https://openscad.org), so every size is a number at the top of the file. If something doesn't fit, change the number and export again.

## Optics bench (print this first)

A rail with a millimeter scale and sliding holders for the screen, the lens and the beam splitter. You slide the holders until the floating text is sharp, then read the distances off the scale. Those numbers decide the size of the module that goes on the glasses.

![Optics bench, assembled](images/assembly.png)

The yellow line is the light: from the screen, through the lens, into the beam splitter, then out the front toward your eye. Each holder has a pointer over the scale:

| Holder | Its pointer marks |
|---|---|
| Screen holder | the screen's surface |
| Lens holder | the middle of the lens |
| Splitter holder (plate or cube) | the center of the splitter, where the light turns |

### What to print

![All parts laid out as they print](images/print_layout.png)

All files in [`stl/`](stl) are already lying the right way for printing. **No supports needed.**

| File | Notes |
|---|---|
| `fit_test.stl` | **Print first.** A short rail and a bare holder base to check the sliding fit (see below) |
| `rail.stl` | 150 mm long, fits a 180 mm bed |
| `oled_holder.stl` | For the Adafruit 0.96" OLED (29.2 × 26.7 mm board) |
| `lens_holder_25mm.stl` | Pick the one that matches your lens diameter: 20, 25, 30 or 34 mm. Print one per lens you want to try |
| `plate_holder.stl` | Holds a flat splitter at 45°: the recommended 25 mm round plate, a microscope slide or clear plastic (up to 1.2 mm thick) |
| `cube_holder_15mm.stl` | Only if you have a 15 mm beam splitter cube. Print it in black if you can: its back wall soaks up stray light |

**Settings:** PLA or PETG, 0.2 mm layers (0.16 mm makes the scale numbers crisper), 3 walls, 15–20% infill. All the parts together use roughly 50 g of filament.

**Make the scale easy to read:** either add a filament color change at **7.4 mm** height when slicing the rail, or rub a fine marker over the engraved ticks and wipe the surface clean.

### Check the fit first

Print `fit_test.stl` and slide the small block onto the short rail; the ends of the ridge are tapered to help it on. Each holder has a small spring finger inside its groove that grips the ridge.

- **Just right:** slides with a gentle push and stays put when you let go.
- **Too tight:** in OpenSCAD, raise `clearance` from 0.25 to 0.3 and export the parts again (or sand the ridge lightly).
- **Too loose:** raise `press` from 0.3 to 0.45.

### Put it together

1. Slide the screen holder, lens holder and splitter holder onto the rail in that order, pointers on the side with the scale.
2. **Screen:** press the OLED into its holder face out. The small lips on the side brackets click over the board's edges. The cable leaves through the open side.
3. **Lens:** press it into the holder from the side with the wide opening; three thin ribs grip it. If one side of the lens is flat, put the flat side toward the screen.
4. **Splitter:** stand the plate in its slot. (A cube sits on its little table; turn it until the screen's light comes out toward the scale side, where your eye goes.)

Then follow the steps in [docs/optics.md](../docs/optics.md).

### Different parts?

Open `optics_bench.scad` in OpenSCAD and show **Window → Customizer**. Enter your measurements (screen board size, lens diameter, cube size, flat splitter thickness), pick a part, press **F6**, then **File → Export → Export as STL**. The `assembly` part is a preview of everything together.

## Next: the glasses module

Once the optics test gives you the distances, the next parts are:

1. **Display module:** a pod for one side of the glasses that holds the screen, lens and beam splitter at your measured distances, with a slot to fine-tune focus.
2. **Battery pod:** holds the Feather and the battery on the arm behind your ear, balancing the weight of the front.
3. **Later:** a fully printed frame with the module built in.

To design them, these measurements are needed:

- The numbers from the optics test, and which lens you used
- The glasses arm where the module clips on: its width and thickness
- Anything that doesn't match the defaults above (board size, cube size)

The battery pod will be sized for the parts list: the Adafruit Feather (50.8 × 22.8 mm) and the 400 mAh battery (36 × 17 × 7.8 mm).
