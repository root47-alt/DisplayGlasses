# Parts list (v1)

Everything plugs together, so **no soldering is needed**. Prices and stock were checked on **Nov 2, 2026**; they change often.

Adafruit parts are also sold by DigiKey, Mouser and Adafruit's local distributors in many countries. Search for the product number (for example "Adafruit 5477") to find one that ships quickly to you.

## Electronics

| Part | Why | Price | Stock (Nov 2) |
|---|---|---|---|
| **Board, option A:** [Adafruit ESP32-S3 Feather, 4 MB Flash / 2 MB PSRAM](https://www.adafruit.com/product/5477) (#5477) | The brain: Bluetooth, battery charger with a battery-level chip, battery plug, and a STEMMA QT port for the screen | $17.50 | Out of stock at Adafruit; check DigiKey/Mouser |
| **Board, option B:** [Adafruit ESP32 Feather V2](https://www.adafruit.com/product/5400) (#5400) | Same plugs and charger, slightly older chip. The firmware supports both (build `feather_v2`) | $19.95 | In stock |
| [Monochrome 0.96" 128×64 OLED, STEMMA QT](https://www.adafruit.com/product/326) (#326) | The screen (white pixels). Plugs into the board with one cable | $17.50 | In stock |
| [STEMMA QT / Qwiic cable, 200 mm](https://www.adafruit.com/product/4401) (#4401) | Connects the screen to the board, long enough to run along the glasses arm | $1.25 | In stock |
| [LiPo battery 3.7 V 400 mAh](https://www.adafruit.com/product/3898) (#3898) | Power. 36 × 17 × 7.8 mm, 8 g. Built-in protection circuit and the right plug polarity for Feathers (many cheap batteries have the plug wired backwards) | $6.95 | In stock |
| USB-C data cable | To program and charge the board (charge-only cables won't work) | — | — |

## Optics

The [optics calculator](../tools/optics/README.md) picked these. A **30 mm lens with a 60 mm focal length and a thin flat beam splitter** shows the whole screen at a comfortable size, with the splitter far enough out to clear your eyelashes. The 15 mm cube on the first version of this list turned out to be too small: the edges of the screen always look dim through it. See [docs/optics.md](optics.md#what-the-calculator-says).

| Part | Why | Price |
|---|---|---|
| **Lens:** plano-convex, **30 mm diameter, 60 mm focal length**, glass. Search for "plano convex lens 30mm 60mm focal length"; science-supply and optics shops sell them | Makes the screen look like a 55-inch TV seen from about 3.3 m. Put the flat side toward the screen | ≈ $8–20 |
| *Optional second lens:* 25 mm diameter, 50 mm focal length, e.g. [Eisco Labs PH0532ACV](https://www.eiscolabs.com/products/ph0532acv) or [Shalom EO 1101-078](https://www.shalomeo.com/Optics/Optical-Lens/BK7-lambda4-Plano-convex-Lens/D25.0mm-F50mm-Uncoated-1101-078) | Bigger text and a shorter module, but your eye has to stay almost exactly centered. Nice to compare on the bench | ≈ $8–20 |
| **Beam splitter:** [Knight Optical BAE2511](https://www.knightoptical.com/stock/plate-beamsplitter-standard-range-50-50-visible-420-680nm-25mmdia-x-1-1mmthk-bae2511), 25 mm round, 1 mm thick, 50/50, anti-reflection coated back | Bounces the screen into your eye while you see through it. Weighs about 1 g. The coated back stops a faint double image | $39.34 (in stock) |

**Cheaper ways to start:**
- **Free:** a glass microscope slide or a piece of clear plastic works as a splitter on the bench. It reflects only about 8% of the light, so it only works indoors with the lights low.
- **Teleprompter glass:** shops like [Two Way Mirrors](https://www.twowaymirrors.com/beamsplitter-mirror/) and [telepromptermirror.com](https://telepromptermirror.com/?p=35216) cut 30% reflective glass to size. It's usually 3 mm thick, so it's heavier and shows a slight double image, but it's fine for the bench.

## Glasses and printing

| Part | Why | Price |
|---|---|---|
| Cheap non-prescription glasses (clear) or sunglasses | The display module clips onto them until we print a full frame. Tinted lenses make the display much easier to see outdoors | $5–10 |
| PLA or PETG filament | For the 3D-printed parts (PETG is tougher and handles a hot car better) | — |

**Total:** about $75–95 with option B, plus shipping.

## Tools

- Your 3D printer
- Calipers (or a ruler), to measure parts for the 3D models
- Double-sided tape or Blu-Tack

## Cheaper option (needs a little soldering)

A Seeed XIAO ESP32-S3 plus a generic 0.96" SSD1306 OLED and a small LiPo costs about $25 instead of $45 for the electronics, but the battery wires have to be soldered to pads on the board. The firmware works on it with a small config change. Ask if you want to go that way.
