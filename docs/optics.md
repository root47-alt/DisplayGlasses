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

The [optics calculator](../tools/optics/README.md) models the screen, lens and splitter before you buy anything.

**Your eyelashes set the first limit.** The splitter is tilted 45°, so its near edge comes about 9 mm closer to you than its center. Its center has to sit about 25 mm in front of your pupil, or that edge brushes your eyelashes. That's a little farther than a normal glasses lens, and it makes the image harder to keep in view, so the lens has to be bigger.

With the splitter's center 25 mm from the pupil:

| Setup | Image size | Eye box (sideways × up/down) | Verdict |
|---|---|---|---|
| 40 mm lens + 15 mm cube | 31° × 16° | 0 × 3.5 mm | Edges always dim; image very wide |
| 50 mm lens, 25 mm wide + 25 mm plate | 25° × 12.5° | 0.7 × 10.8 mm | Too fussy: shift 1 mm and an edge dims |
| 50 mm lens, 30 mm wide + 20 mm cube | 25° × 12.5° | 3.7 × 9.8 mm | Works, but the cube weighs 20 g |
| **60 mm lens, 30 mm wide + 25 mm plate** | **21° × 10.5°** | **4.5 × 16.5 mm** | **Recommended** |
| 60 mm lens, 30 mm wide + 30 × 22 mm plate | 21° × 10.5° | 6.9 × 13.5 mm | Roomiest |

What that means:

- **Use a lens 30 mm wide with a 60 mm focal length.** The screen looks like a 55-inch TV seen from about 3.3 m, and capital letters look about 20 mm tall on a sign 1 m away. That's still easy to read.
- **Use a thin flat beam splitter, not the 15 mm cube.** A 25 mm plate weighs under 2 g and shows the whole screen. A 15 mm cube is too small: the left and right edges always look dim.
- **The screen sits about 58 mm from the lens**, and it stays sharp anywhere from 53 to 60 mm (text floating from 0.5 m out to infinity). The glasses module needs a focus slot about 7 mm long.
- **The light path from screen to splitter is about 71 mm.** That's longer than the space beside your eye, so the module sticks out past the side of your head, or folds the light with small mirrors. See [cad/README.md](../cad/README.md).
- **The eye box is about 4.5 mm sideways.** The glasses need to sit in the same place on your face every time, so a snug fit matters. A 30 × 22 mm plate stretches that to about 7 mm.

## Test it on the optics bench (about 30 minutes)

Print the bench first ([cad/README.md](../cad/README.md)) and put the screen, lens and beam splitter in their holders. The OLED should be running the `feather_demo` firmware. No printer handy? Cardboard and Blu-Tack work too; you'll just measure the distances with a ruler.

1. **Raise the bench to eye level** by setting it on a box or a stack of books, so you can look into the cube from the front without bending down.
2. **Look into the splitter** from the scale side of the bench, past it toward the room. The screen's light should be visible in it. With a cube, if you see nothing, give it a quarter turn or set it down on a different face until the light comes out toward you (the mirror inside a cube is a diagonal, and it has to stand upright and face you).
3. **Focus.** Slide the lens holder (or the screen holder) slowly along the rail. Stop when the text looks sharp while your eyes are relaxed, as if you were looking across the room. Then glance at something far away and back: the text should stay sharp without your eyes refocusing.
4. **Fix the mirror image.** If the text reads backwards, hold the button for 1–2 seconds and let go. That cycles the mirror modes (none → left-right → up-down → rotate 180). The choice is saved.
5. **Check that you can see the whole screen.** Slide the splitter holder right up to the lens holder and put your eye about 15 mm from the splitter, which is about where it will sit in the glasses. The closer the lens is to your eye, the more of the screen you see. If the edges of the text are still cut off, try a bigger lens or one with a longer focal length. The calculator predicts this, so compare what you see with it.
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
