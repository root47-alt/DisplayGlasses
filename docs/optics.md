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

## Test it on the optics bench (about 30 minutes)

Print the bench first ([cad/README.md](../cad/README.md)) and put the screen, lens and cube in their holders. The OLED should be running the `feather_demo` firmware. No printer handy? Cardboard and Blu-Tack work too; you'll just measure the distances with a ruler.

1. **Raise the bench to eye level** by setting it on a box or a stack of books, so you can look into the cube from the front without bending down.
2. **Look into the cube** from the scale side of the bench, past it toward the room. The screen's light should be visible in the cube. If you see nothing, give the cube a quarter turn, or set it down on a different face, until the light comes out toward you. The mirror plane inside the cube is a diagonal, and it has to stand upright and face you.
3. **Focus.** Slide the lens holder (or the screen holder) slowly along the rail. Stop when the text looks sharp while your eyes are relaxed, as if you were looking across the room. Then glance at something far away and back: the text should stay sharp without your eyes refocusing.
4. **Fix the mirror image.** If the text reads backwards, hold the button for 1–2 seconds and let go. That cycles the mirror modes (none → left-right → up-down → rotate 180). The choice is saved.
5. **Check that you can see the whole screen.** Slide the cube holder right up to the lens holder and put your eye about 15 mm from the cube, which is about where it will sit in the glasses. The closer the lens is to your eye, the more of the screen you see. If the edges of the text are still cut off, try a bigger lens or one with a longer focal length.
6. **Read the pointers and write down:**
   - screen-to-lens distance = lens pointer − screen pointer
   - lens-to-cube distance = cube pointer − lens pointer
   - which lens you used (diameter and focal length) and the mirror mode that reads correctly

Repeat for each lens you bought. A longer focal length gives smaller text that's easier on your eyes, but makes the module stick out farther. The screen-to-lens distance should come out a little shorter than the lens's focal length.

## Tips

- **Outdoors:** daylight drowns out the display. Tinted lenses (sunglasses) help a lot.
- **Too bright at night:** lower `SCREEN_BRIGHTNESS` in `firmware/src/config.h`.
- **Double image:** a flat glass plate reflects off both of its faces. A cube splitter avoids that.
- **Placement:** put the display a little above or to the side of where you normally look, so it doesn't block your view. Glancing at it should feel like checking a car's mirror.
