# Initialization and display control

Brightness, power and e-paper features depend on the display hardware. Read the device guide before choosing refresh behavior.

- [Initialization](#initialization)
- [Orientation and dimensions](#orientation-and-dimensions)
- [Clipping and scrolling](#clipping-and-scrolling)
- [Brightness and power](#brightness-and-power)
- [Refresh and e-paper](#refresh-and-e-paper)

## Initialization

`init()`, `begin()`.

## Orientation and dimensions

`setRotation()`, `getRotation()`, `width()`, `height()`.

Set the logical drawing orientation of a display. Use this before drawing a new
layout, then obtain the current dimensions with `width()` and `height()`.

### Signature

```cpp
void setRotation(uint_fast8_t rotation);
uint8_t getRotation(void) const;
```

These are public methods of `lgfx::LGFXBase`, inherited by `M5GFX`.

### Parameters and return value

- `rotation`: use **0–7** for the LCD mapping documented here. Values **0–3**
  select quarter-turn rotations; values **4–7** add a reflection.
- There is no default argument. The value after `init()` is device-specific.
- `setRotation()` returns **none (`void`)**. `getRotation()` returns the current
  logical rotation value, not the internal panel mounting offset.

The LCD implementation masks the argument with `& 7`. Pass an explicit value
in the documented range instead of relying on this normalization across backends.

### Eight orientations of F

![A fixed screen showing F for setRotation 0 through 7, including mirrored views,
logical origins, axis arrows, and width/height labels.](../assets/setRotation/rotation-f.svg)

**Read the figure from the front of the screen. Keep the screen still.** The
upright F at `setRotation(0)` defines the reference pose; it is not a claim that
rotation 0 matches every device's normal chassis orientation. The F is redrawn
after each call. `W` and `H` are the dimensions reported at rotation 0.

The figure describes the ordinary non-mirrored mounting case
(`offset_rotation` 0–3), including the three LCD devices below. A custom mounting
with the mirror bit set needs its own verified mapping.

| Value | Operation on the upright F | Origin on the fixed screen | +X | +Y | `width()` × `height()` |
| --- | --- | --- | --- | --- | --- |
| `0` | No rotation | Top left | Right | Down | W × H |
| `1` | 90° clockwise | Top right | Down | Left | H × W |
| `2` | 180° | Bottom right | Left | Up | W × H |
| `3` | 270° clockwise | Bottom left | Up | Right | H × W |
| `4` | Reflect top to bottom | Bottom left | Right | Up | W × H |
| `5` | Reflect top to bottom, then rotate 90° clockwise | Top left | Down | Right | H × W |
| `6` | Reflect top to bottom, then rotate 180° | Top right | Left | Down | W × H |
| `7` | Reflect top to bottom, then rotate 270° clockwise | Bottom right | Up | Left | H × W |

Here, **reflect top to bottom** means reversing logical Y before rotating.
In particular, `4` is an upside-down reflection, and `6` is a left/right
reflection of the reference F. **4–7 are not a second set of counterclockwise
rotations.** The corner colors identify the same logical corners in every view:
orange = top left, green = top right, blue = bottom left, pink = bottom right.

All drawing coordinates remain logical: `(0, 0)` is the drawing origin,
`(width() - 1, height() - 1)` is the opposite logical corner. Their positions on
the fixed physical screen change with rotation. On a square display the
dimensions stay equal even though the image direction changes.

### Example

This complete Arduino sketch draws an F at rotation 1 without depending on a
particular display resolution:

```cpp
#include <M5GFX.h>

M5GFX display;

void setup()
{
  display.init();
  display.setRotation(1);
  const int w = display.width();
  const int h = display.height();
  const int u = ((w < h) ? w : h) / 9;
  const int x = (w - 3 * u) / 2;
  const int y = (h - 5 * u) / 2;
  display.fillScreen(TFT_BLACK);
  display.fillRect(x, y, u, 5 * u, TFT_WHITE);
  display.fillRect(x, y, 3 * u, u, TFT_WHITE);
  display.fillRect(x, y + 2 * u, 2 * u, u, TFT_WHITE);
  display.display();
}

void loop() {}
```

Run [DisplayRotation](../../examples/Basic/DisplayRotation/DisplayRotation.ino)
to cycle through all eight views. It uses the same F geometry and corner pattern
as the diagram, adds logical X/Y arrows, and prints the startup rotation and
each view's dimensions to Serial at 115200 baud. Its timed cycle is intended for
LCDs; the sketch stops on an e-paper display.

### State changes and redraws

- Call `init()` before querying dimensions or applying an application layout.
- Calling `setRotation()` clears the clip rectangle and scroll rectangle back
  to the full logical display, even when the requested rotation is unchanged.
  Reapply custom rectangles afterward.
- Use the new `width()` and `height()` for layout. Odd values exchange the
  dimensions relative to rotation 0 for the mapping above.
- The function configures subsequent drawing. It does not perform a general
  software rotation of an existing framebuffer or redraw the scene. Clear and
  redraw; use `display()` when the backend requires a display update.
- Display rotation and sprite rotation are separate state. Rotating a sprite
  does not call `setRotation()` on its destination display.

### Device differences

See the [device orientation reference](../device-orientation.md) for a
separate proposal to align rotation zero with a fixed chassis reference. The
values below describe current behavior; that proposal has not changed them.

The following values describe **M5GFX initialization alone**, before application
code or another library changes rotation. The table describes the v0.2.29 defaults.

| Device | LCD driver | `offset_rotation` | Startup `getRotation()` | Startup width × height | Rotation 0 width × height |
| --- | --- | --- | --- | --- | --- |
| CoreS3 | ILI9342 | 3 | 1 | 320 × 240 | 240 × 320 |
| StickC Plus2 | ST7789 | 0 | 0 | 135 × 240 | 135 × 240 |
| ChainCaptain | JD9853 | 2 | 0 | 240 × 240 | 240 × 240 |

For a reproducible device reference, draw an F immediately after `init()` without
changing rotation. Hold the device so that this F is upright, then keep it still:

- **CoreS3:** values 0, 1, 2, 3 produce a 90° counterclockwise F, an upright F,
  a 90° clockwise F, and a 180° F, respectively, relative to that startup view.
  Rotate the entire generic chart 90° counterclockwise to compare it with this
  device pose. The generic chart itself uses rotation 0 as its reference.
- **StickC Plus2:** the startup view is rotation 0; the generic chart's reference
  already matches it. Width and height exchange at odd values.
- **ChainCaptain:** the startup view is also rotation 0. Its square screen keeps
  the same dimensions for every value, so use the F and corner markers to check
  direction rather than the dimensions alone.

#### Why the panel offset matters

`Panel_LCD::setRotation()` combines the public value with the board configuration:

```cpp
r &= 7;
internal_rotation = ((r + offset_rotation) & 3)
                  | ((r & 4) ^ (offset_rotation & 4));
```

The panel uses this internal value to select controller address directions and
dimensions. `getRotation()` still reports `r`. A default value of 1, an internal
value of 0, and a normally oriented image can therefore all describe the same
CoreS3 state. Do not remove a board's offset to make its raw controller values
match the generic chart.

This rotation reference describes the LCD behavior above. E-paper, external displays, custom
panel drivers, and mirrored mounting configurations must be checked against
their own overrides; a signature shared with `LGFXBase` is not proof of identical
hardware behavior.

For manual hardware checks, use [DisplayRotationButton](../../examples/Basic/DisplayRotationButton).
Press A to advance through 0–7, or select a value over serial. This variant
requires M5Unified for button handling.

## Clipping and scrolling

`setClipRect()`, `clearClipRect()`, `setScrollRect()`, `scroll()`.

## Brightness and power

`setBrightness()`, `sleep()`, `wakeup()`.

## Refresh and e-paper

`display()`, `waitDisplay()`, `isEPD()`, `setEpdMode()`.

## Learn more

See also [device orientation](../device-orientation.md) and [device guides](../devices/README.md).

[Public declarations](../../src/lgfx/v1/LGFXBase.hpp)

[API index](README.md) · [Documentation home](../README.md)
