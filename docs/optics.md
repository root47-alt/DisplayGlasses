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

## Test it on the desk (about 30 minutes)

You need the OLED running the `feather_demo` firmware, one lens, the beam splitter, cardboard, and tape or Blu-Tack.

1. **Focus.** In a dim room, look through the lens at the OLED. Start with the screen right against the lens and slowly move the lens away. Stop when the text looks sharp while your eyes are relaxed, as if looking across the room. The screen-to-lens distance should end up a little shorter than the lens's focal length.
2. **Fold it.** Turn the screen and lens sideways and put the beam splitter in front of your eye, angled 45°, so the light coming out of the lens turns 90° into your eye. Look straight ahead through the splitter. The text should float over the room.
3. **Fix the mirror image.** If the text reads backwards, hold the button for 1–2 seconds and let go. That cycles the mirror modes (none → left-right → up-down → rotate 180). The choice is saved.
4. **Measure and write down:**
   - screen-to-lens distance
   - lens-to-splitter distance
   - the lens diameter and the splitter cube size
   - which lens you used, and the mirror mode that reads correctly

Try each lens: a longer focal length gives smaller, easier-to-read text but makes the module stick out farther.

## Tips

- **Outdoors:** daylight drowns out the display. Tinted lenses (sunglasses) help a lot.
- **Too bright at night:** lower `SCREEN_BRIGHTNESS` in `firmware/src/config.h`.
- **Double image:** a flat glass plate reflects off both of its faces. A cube splitter avoids that.
- **Placement:** put the display a little above or to the side of where you normally look, so it doesn't block your view. Glancing at it should feel like checking a car's mirror.
