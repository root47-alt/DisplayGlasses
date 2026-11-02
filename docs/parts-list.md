# Parts list (v1)

Everything plugs together, so **no soldering is needed**. Prices are approximate (USD).

## Electronics

| Part | Why | ≈ Price |
|---|---|---|
| **Adafruit ESP32-S3 Feather** (4 MB Flash / 2 MB PSRAM version) | The brain. Bluetooth, a built-in LiPo charger, a battery plug and a STEMMA QT port for the screen | $17.50 |
| **Adafruit Monochrome 0.96" 128×64 OLED, STEMMA QT** | The screen. Plugs into the Feather with one cable | $17.50 |
| **STEMMA QT / Qwiic cable, 200 mm** | Connects screen to board, long enough to run along the glasses arm | $1 |
| **Adafruit LiPo battery 3.7 V, 400 mAh (JST-PH plug)** | Power. Adafruit's batteries have the right plug polarity for the Feather and a protection circuit; many cheap ones have the plug wired backwards | $7 |
| USB-C data cable | To program and charge the board (charge-only cables won't work) | — |

> Got the **8 MB Flash / no PSRAM** Feather instead? It works too: in `firmware/platformio.ini`, change `board = adafruit_feather_esp32s3` to `board = adafruit_feather_esp32s3_nopsram`.

## Optics

| Part | Why | ≈ Price |
|---|---|---|
| **2–3 small convex lenses**, 20–30 mm wide, focal length about 30–50 mm (Google Cardboard replacement lenses work) | Makes the screen focusable. Buy a few so you can test which looks best | $10 |
| **50/50 beam splitter cube, ~15 mm** | The see-through reflector. A cube is easier to mount than a glass plate and has no double image | $10–15 |

A piece of clear plastic or a glass microscope slide also works as a beam splitter for a first test. It reflects much less light, so it only works indoors.

## Glasses and printing

| Part | Why | ≈ Price |
|---|---|---|
| Cheap non-prescription glasses (clear) or sunglasses | The display module clips onto them. Tinted lenses make the display much easier to see outdoors | $5–10 |
| PLA or PETG filament | For the 3D-printed module (PETG is tougher and handles a hot car better) | — |

**Total: about $70–90.**

## Tools

- Your 3D printer
- A small screwdriver set and some double-sided tape or Blu-Tack for the optics test
- Optional: calipers, to measure the parts for the 3D model (a ruler works too)

## Cheaper option (needs a little soldering)

A Seeed XIAO ESP32-S3 plus a generic 0.96" SSD1306 OLED and a small LiPo costs about $25 instead of $45 for the electronics, but the battery wires have to be soldered to pads on the board. The firmware works on it with a small config change. Ask if you want to go that way.
