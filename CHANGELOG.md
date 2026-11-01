# Changelog

## 0.1.0 (1 November 2026)

First release.

- **Starter profile** (`profiles/Hercules MP3 e2.xml`) with 52 bindings:
  - **Deck A, receive:** VFO tuning on the jog wheel, plus the other receive controls.
  - **Deck B, transmit:** MOX, TUNE, ATU and the other transmit controls.
  - **Centre:** Band ▲ / Band ▼ on the arrow buttons.
- **Web mapper** (`web-mapper/index.html`):
  - Click-to-map picture of the MP3 e2, with each control's function shown above it.
  - Function list filtered to what each control can drive.
  - Loops/Fx key banks, an optional photo with movable labels, and profile export and import.
- **AetherSDR Visual Mapper patch** (`aethersdr-patch/`):
  - A **Visual Mapper…** window in the MIDI Mapping dialog that edits the live bindings.
  - JSON controller layouts, with the MP3 e2 layout bundled.
  - Touching a control on the hardware selects it.
  - Made against AetherSDR commit `330ef344`. Its new test passes all 16 checks.
  - The window itself has not yet been tested on hardware.
- **Documentation:** MIDI map, design, Windows build guide, project log and maintenance guide.
