# 🧩 New Feature: Sprite Kitchen (Dynamic Graphics Cooking)
**FINALBYTE introduces Sprite Kitchen, a dynamic system that allows creating new sprite banks during game loading by combining and transforming existing graphics.**

## Basic operations include:
- Shift (pixel movement)
- Mirror (horizontal and vertical flipping)
- Rotate (90°, 180°, 270°)
- Palette swap (color remapping)
- Overlay and mask merge (layer blending, transparency effects)
- Scale (50% shrink, 200% zoom)

Developers define "sprite recipes" in the game manifest, combining multiple steps in sequence.
As a result, a small set of base sprites can be expanded into rich, unique banks with minimal data overhead.

All transformations are handled by the FINALBYTE module (ESP32/RP2040) during loading, without affecting the host CPU performance.

This approach saves memory, speeds up game development, and opens up massive visual diversity while keeping games lightweight.

# 🎶 New Feature: Sound Kitchen (Dynamic Sample Processing)
**Alongside graphics, FINALBYTE also introduces the Sound Kitchen, allowing dynamic sound sample processing at load time.**

Supported audio operations include:
- Crop (select a sample range)
- Reverse (playback backwards)
- Volume adjustment (amplify or attenuate)
- Speed change (pitch up or down by simple resampling)
- Mixing (blend two samples with a custom ratio)

Like sprites, "sound recipes" are defined in the game manifest.
FINALBYTE processes them during game initialization, generating new sounds from a few base samples.

This system dramatically increases audio variety while minimizing storage size, allowing dynamic sound effects, voice variations, and environment changes without taxing the host system.
