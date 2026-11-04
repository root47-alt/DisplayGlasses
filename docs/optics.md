# Optics guide

The goal is to find three distances that make the text sharp and floating out in front of you. Those numbers go into the 3D-printed module.

## Why the lens is needed

Your eye can't focus on anything closer than about 10 cm. A convex lens placed so the screen sits just inside its **focal length** makes the screen look large and far away, where your eye focuses easily. The beam splitter then folds that image into your line of sight.

```
       world
         |
         v
         / <---- () <---- [OLED]
         |       lens     (just inside one focal
         v                 length from the lens)
        eye       / = beam splitter at 45°
```

This is a top view of your right eye, looking up the page. The display sits out by your temple, facing your nose. The splitter in front of your eye turns its light 90° into your eye, and you still see the world through the splitter.

## What the calculator says

The [optics calculator](../tools/optics/README.md) models the screen, lens and splitter before you buy anything. It traces light from your pupil to the edges and corners of the screen.

**Your eyelashes set the first limit.** The splitter is tilted 45°, so its near edge comes closer to you than its center: about 9 mm closer for a 25 mm plate. That edge should stay about 16 mm from your pupil, or it brushes your eyelashes. So a 25 mm plate's center sits about 25 mm in front of your pupil, a little farther than a normal glasses lens, and a bigger plate has to sit farther out still. The farther out the splitter, the harder the image is to keep in view, so the lens has to be bigger.

Each setup below sits as close as its near edge allows. **Eye box** is how far your eye can move with the whole screen in view at full brightness, corners included. **Half bright** is how far it can move before an edge or corner drops to about half brightness. Plate sizes are width × height; the width is the side that tilts.

| Setup | Splitter center from pupil | Image size | Eye box (sideways × up/down) | Half bright | Verdict |
|---|---|---|---|---|---|
| 40 mm lens + 15 mm cube | 23.7 mm | 31° × 16° | 0 × 0 mm | 0.7 × 7.9 mm | Edges always dim; image very wide |
| 50 mm lens, 25 mm wide + 25 mm round plate | 25 mm | 25° × 12.5° | 0 × 0 mm | 3.6 × 7.7 mm | Too fussy |
| 60 mm lens, 30 mm wide + 20 mm round plate | 23.3 mm | 21° × 10.5° | 1.2 × 0 mm | 5.2 × 8.3 mm | Too small: the corners always dim a little |
| **60 mm lens, 30 mm wide + 25 mm round plate** | **25 mm** | **21° × 10.5°** | **4.3 × 4.2 mm** | **8.3 × 13.8 mm** | **Recommended: affordable and works** |
| 60 mm lens, 30 mm wide + 25 × 25 mm square plate | 25 mm | 21° × 10.5° | 4.5 × 12.7 mm | 8.5 × 17.4 mm | Square beats round |
| 60 mm lens, 30 mm wide + 35 × 25 mm plate | 28.6 mm | 21° × 10.5° | 6.0 × 9.5 mm | 10.1 × 14.5 mm | Ideal plate |
| 60 mm lens, 35 mm wide + 35 × 25 mm plate | 28.6 mm | 21° × 10.5° | 8.6 × 14.7 mm | 12.7 × 18.7 mm | Roomiest |
| 60 mm lens, 30 mm wide + 45 × 25 mm plate | 32.1 mm | 21° × 10.5° | 3.2 × 5.4 mm | 7.5 × 11.2 mm | Too big: it has to sit too far out |
| 50 mm lens, 30 mm wide + 20 mm cube | 26.2 mm | 25° × 12.5° | 3.2 × 9.6 mm | 7.2 × 13.6 mm | Works, but the cube weighs 20 g |

What that means:

- **Use a lens 30 mm wide with a 60 mm focal length.** The screen looks like a 55-inch TV seen from about 3.3 m, and capital letters look about 20 mm tall on a sign 1 m away. That's still easy to read.
- **Use a thin flat beam splitter, not the 15 mm cube.** A 25 mm plate weighs about 1 g and shows the whole screen. A 15 mm cube is too small: the edges always look dim.
- **The affordable 25 mm round plate works.** Your eye can move about 4 × 4 mm with no dimming at all, and about 8 × 14 mm before an edge or corner is half as bright. The glasses need to sit in the same place on your face every time, so a snug fit matters.
- **A rectangular plate about 35 × 25 mm is ideal**, with the long side horizontal and its center about 28.6 mm from your pupil. It gives 6 × 9.5 mm with no dimming, or 8.6 × 14.7 mm with a 35 mm lens. Three things decide the shape:
  - The 45° tilt makes the splitter look about 30% narrower from your eye, while the screen's image is twice as wide as it is tall. So the long side goes horizontal.
  - The image's corners pass through the plate's corners, which a round plate doesn't have. A square or rectangular plate keeps the corners bright longer than a round one of the same size.
  - Past about 40 mm wide, a bigger plate stops helping: its near edge comes toward your eye, so the whole plate has to sit farther out.
- **The screen sits about 58 mm from the lens**, and it stays sharp anywhere from 53 to 60 mm (text floating from 0.5 m out to infinity). The glasses module needs a focus slot about 7 mm long.
- **The light path from screen to splitter is about 71 mm** (75 mm with a 35 × 25 mm plate). That's longer than the space beside your eye, so the module sticks out past the side of your head, or folds the light with small mirrors. See [cad/README.md](../cad/README.md).

## Choosing a beam splitter

- **50/50 and non-polarizing.** Half the screen's light reaches your eye, and you see the world at half brightness. Anything from 30/70 (brighter world, dimmer text) to 60/40 works.
- **About 1 mm thick.** The printed slots take plates up to about 1.4 mm. A thicker plate works optically if the text floats 1 m or farther, because the faint second image from its back surface lands less than a pixel away. But it's heavier, and the slot needs a new `plate_t` in OpenSCAD.
- **Coated side toward the screen and your eye.** To find it, touch a pencil tip to the glass. On the coated side, the bright reflection touches the tip; on the other side there's a small gap, the thickness of the glass.
- **Plain glass** (a microscope slide, clear plastic) reflects only about 10% of the light. It's fine on the bench in a dim room, but too faint for the glasses.
- **Not a polarizing cube.** It reflects or passes light depending on its polarization, so depending on the angle, the screen's image or the phone and laptop screens you look at through it can go dark.
- **A non-polarizing cube of 20–25 mm** works on the bench, but at 20–40 g it's too heavy for the glasses.

## Test it on the optics bench (about 30 minutes)

Print the bench first ([cad/README.md](../cad/README.md)) and put the screen, lens and beam splitter in their holders. The OLED should be running the `feather_demo` firmware. No printer handy? Cardboard and Blu-Tack work too; you'll just measure the distances with a ruler.

1. **Raise the bench to eye level** by setting it on a box or a stack of books, so you can look into the splitter from the front without bending down.
2. **Look into the splitter** from the scale side of the bench, past it toward the room. The screen's light should be visible in it. With a cube, if you see nothing, give it a quarter turn or set it down on a different face until the light comes out toward you (the mirror inside a cube is a diagonal, and it has to stand upright and face you).
3. **Focus.** Slide the lens holder (or the screen holder) slowly along the rail. Stop when the text looks sharp while your eyes are relaxed, as if you were looking across the room. Then glance at something far away and back: the text should stay sharp without your eyes refocusing.
4. **Fix the mirror image.** If the text reads backwards, hold the button until the screen says **Flip image** (about 5 seconds; it says **Turn off** first, so keep holding), then let go. That cycles the mirror modes (none → left-right → up-down → rotate 180). The choice is saved.
5. **Check that you can see the whole screen.** Set the splitter 12.8 mm past the lens (splitter pointer − lens pointer), where it sits in the glasses; the holders can get as close as 12 mm. Put your eye about 25 mm from the middle of the splitter (28.6 mm for a 35 × 25 mm plate), where it sits in the glasses. The closer the lens is to your eye, the more of the screen you see. Move your head a few millimeters sideways and up and down: with the 25 mm round plate, the corners should stay bright for about 4 mm. If the edges or corners of the text are cut off, try a bigger lens or one with a longer focal length. The calculator predicts this, so compare what you see with it.
6. **Read the pointers and write down:**
   - screen-to-lens distance = lens pointer − screen pointer
   - lens-to-splitter distance = splitter pointer − lens pointer
   - which lens you used (diameter and focal length) and the mirror mode that reads correctly

Repeat for each lens you bought. A longer focal length gives smaller text that's easier on your eyes, but makes the module stick out farther. The screen-to-lens distance should come out a little shorter than the lens's focal length.

## Tips

- **Outdoors:** daylight drowns out the display. Tinted lenses (sunglasses) help a lot.
- **Too bright at night:** lower `SCREEN_BRIGHTNESS` in `firmware/src/config.h`.
- **Double image:** a plain glass plate reflects off both faces, which can show a faint second image. A plate with an anti-reflection coated back (like the one on the parts list) avoids most of it, and focusing the text 1 m or farther lines the two up.
- **Placement:** put the display a little above or to the side of where you normally look, so it doesn't block your view. Glancing at it should feel like checking a car's mirror.
