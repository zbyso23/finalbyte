# FINALBYTE — Sprite and Samples Kitchen
## Revision 1.3

FINALBYTE provides two load-time asset processing systems: **Sprite Kitchen** for graphics and **Sample Kitchen** for audio.

Both systems generate derived assets from a small set of source assets during game initialization or bank loading. The resulting assets are stored in FINALBYTE memory and used normally at runtime. Kitchen processing does **not** consume host CPU time during gameplay.

The Kitchen is intentionally designed as a lightweight **asset compiler**, not as a general-purpose image or audio editor.

---

# 🧩 Sprite Kitchen

Sprite Kitchen creates new sprites, tiles, masks, animation frames and graphic banks by combining and transforming existing graphics.

Developers define **sprite recipes** in the game manifest. A recipe is an ordered sequence of simple operations applied to one or more source images.

## 1. Geometry operations

Supported operations:

- **SHIFT(x, y)** — move image content by a pixel offset.
- **MIRROR_X** — horizontal flip.
- **MIRROR_Y** — vertical flip.
- **ROTATE_90** — rotate 90° clockwise.
- **ROTATE_180** — rotate 180°.
- **ROTATE_270** — rotate 270° clockwise.
- **SCALE_50** — shrink to 50% using simple nearest/palette-safe resampling.
- **SCALE_200** — enlarge to 200% using nearest-neighbour pixel replication.
- **CROP(x, y, w, h)** — extract a rectangular region.
- **SLICE(w, h, count)** — split a sprite sheet into equally sized frames.
- **TRIM** — remove fully transparent borders and retain the visible bounding box.
- **REPEAT_X(n)** — repeat the source horizontally.
- **REPEAT_Y(n)** — repeat the source vertically.

Geometry operations are intentionally limited to deterministic, inexpensive transforms. Arbitrary-angle rotation and filtered scaling are outside the core Kitchen profile.

## 2. Palette and transparency operations

- **PALETTE_SWAP(map)** — remap palette indices.
- **COLOR_KEY(color)** — convert one selected color or palette index to transparent.
- **MASK(source)** — apply a binary transparency mask.
- **MASK_MERGE(source)** — combine an image with a mask source.

Sprite Kitchen treats transparency as asset data. This is independent from the FINALBYTE host-video compositor.

## 3. Blend and composite operations

Sprite Kitchen supports inexpensive two-input blend operations.

The second input may be:

1. another sprite/image, or
2. a constant color plane.

Core constant planes:

- `BLACK`
- `WHITE`
- `RED`
- `GREEN`
- `BLUE`
- `CYAN`
- `MAGENTA`
- `YELLOW`

Supported blend modes:

- **COPY / NORMAL** — copy or normal source-over composition.
- **MULTIPLY** — darken/tint by multiplication.
- **SCREEN** — brighten using inverse multiplication.
- **OVERLAY** — contrast-preserving darken/lighten combination.
- **ADD** — additive blend with clamping.
- **SUBTRACT** — subtractive blend with clamping.
- **MIN** — component-wise minimum.
- **MAX** — component-wise maximum.
- **XOR** — inexpensive logical/palette effect.

A blend operation may optionally specify **strength 0–15**. Strength is evaluated only while generating the derived asset; the resulting sprite is stored as an ordinary finished asset.

Example recipes:

```text
soldier_night =
    SOURCE soldier
    BLEND MULTIPLY, BLUE, strength=8

soldier_hit =
    SOURCE soldier
    BLEND ADD, RED, strength=5

soldier_frozen_right =
    SOURCE soldier
    PALETTE_SWAP frost_palette
    BLEND SCREEN, CYAN, strength=4
    MASK_MERGE frost_mask
    MIRROR_X
```

For palette-based modes, FINALBYTE may accelerate repeated blend operations through palette lookup tables. The output is quantized to the active destination palette.

## 4. Composition

- **MERGE(source, x, y)** — place a second image at an offset.
- **OVERLAY_IMAGE(source, x, y)** — compose a second image using its transparency information.
- **MASKED_OVERLAY(source, mask, x, y)** — compose through an explicit mask.

These operations allow characters, equipment, damage states, shadows, icons and environmental variants to be assembled from reusable components.

## 5. Sprite metadata

Recipes may also define metadata without modifying pixels:

- **ORIGIN(x, y)** — logical sprite origin.
- **HOTSPOT(name, x, y)** — named interaction point such as weapon muzzle, hand, feet or center.
- **COLLISION_BOX(x, y, w, h)** — optional default collision rectangle.

Metadata is retained with the generated asset and may be used directly by the FINALBYTE runtime.

## 6. Processing model

Sprite recipes are executed:

- during game initialization,
- during graphics-bank loading, or
- explicitly when a new derived bank is requested.

They are not intended for continuous per-frame image processing.

Typical flow:

```text
source assets
    ↓
Sprite Kitchen recipes
    ↓
derived sprite/tile bank
    ↓
FINALBYTE runtime renderer
```

This allows a small source set to generate large visual families with minimal storage overhead and no runtime cost to the 8-bit host CPU.

---

# 🎶 Sample Kitchen

Sample Kitchen creates derived sound effects, voices and environmental variations from a small number of source samples.

Like Sprite Kitchen, processing occurs during initialization or bank loading and the resulting samples are stored for normal runtime playback.

## 1. Core sample operations

- **CROP(start, end)** — extract a selected sample region.
- **REVERSE** — reverse sample playback data.
- **VOLUME(gain)** — amplify or attenuate.
- **SPEED(ratio)** — change playback speed using simple resampling; pitch changes with speed.
- **MIX(source, ratio)** — mix two samples with a defined ratio.

## 2. Additional lightweight operations

- **FADE_IN(length)** — linear fade from silence.
- **FADE_OUT(length)** — linear fade to silence.
- **NORMALIZE(target)** — normalize peak level to a selected target.
- **SILENCE(length)** — insert silence.
- **APPEND(source)** — concatenate another sample.
- **LOOP_REGION(start, end)** — define loop metadata without duplicating sample data when possible.

These operations remain intentionally simple and deterministic. Complex DSP belongs to the runtime sound engine rather than to the Kitchen format.

Example:

```text
robot_warning =
    SOURCE warning_voice
    CROP 1200, 18400
    SPEED 0.85
    VOLUME 0.9
    MIX radio_noise, 0.12
    FADE_OUT 400
```

---

# 🔧 Recipes and banks

Sprite and sample recipes are declared in the game manifest and may reference:

- built-in assets,
- game-specific source assets,
- assets from the currently loaded bank,
- previously generated assets where dependency order is unambiguous.

A recipe must always produce a deterministic result from the same input assets and parameters.

Generated assets may be placed into one of the FINALBYTE user banks and used exactly like directly stored assets.

---

# 🖥️ Relationship to FINALBYTE video compositing

Sprite Kitchen blending and FINALBYTE host-video compositing are separate mechanisms.

Inside Sprite Kitchen, images may be blended with modes such as Multiply, Screen, Overlay or Add because this processing occurs while creating a new asset.

Inside the FINALBYTE-generated graphics layer, internal palette transparency and optional alpha effects may also be used by the renderer.

However, compositing between the **original host video signal** and the **FINALBYTE-generated video signal** uses binary pixel selection only:

```text
0 = show original host video
1 = show FINALBYTE video
```

FINALBYTE does not require digitization of the original composite video signal and does not perform true multi-level alpha blending against host pixels.

---

# 🎯 Design principle

The Kitchens exist to multiply useful content, not implementation complexity.

A small number of carefully chosen primitive operations should allow developers to create:

- mirrored animation sets,
- palette variants,
- day/night or elemental variants,
- damage and hit states,
- equipment combinations,
- procedural-looking tile families,
- UI variants,
- voice and sound-effect variations,
- environment-specific audio.

The FINALBYTE host remains responsible for the game. The Kitchen simply prepares richer assets for the FINALBYTE graphics and sound engines without taxing the original 8-bit CPU.
