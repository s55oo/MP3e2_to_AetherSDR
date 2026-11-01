# AetherSDR Visual Mapper patch

Adds a **Visual Mapper…** window to AetherSDR's MIDI Mapping dialog. It shows a picture of the
controller with each control's function written above it, and you set a control by clicking it
and choosing a function.

| File | Use |
|---|---|
| `0001-MIDI-visual-mapper.patch` | The change, made with `git format-patch`. Apply it with `git am` or `git apply`. |
| `BASE_COMMIT.txt` | The AetherSDR commit the patch was made against. |
| `source/` | The same change as plain files, in AetherSDR's folder structure. Copy them over a clone if the patch no longer applies. |

## What it changes

| AetherSDR file | Change |
|---|---|
| `src/core/MidiControllerLayout.{h,cpp}` | New. Reads and validates controller layout files (JSON). |
| `resources/midi-layouts/hercules-djcontrol-mp3-e2.json` | New. The MP3 e2 / MP3 LE / Glow layout. |
| `src/gui/MidiControllerCanvas.{h,cpp}` | New. Draws the controller and handles clicks. |
| `src/gui/MidiVisualMapperDialog.{h,cpp}` | New. The Visual Mapper window. |
| `src/gui/MidiMappingDialog.{h,cpp}` | Adds the **Visual Mapper…** button and keeps the table and picture in sync. |
| `CMakeLists.txt`, `resources/resources.qrc` | Register the new sources and the layout. |
| `tests/midi_controller_layout_test.cpp`, `tests/tests.cmake` | New test (16 checks, all passing). |
| `docs/user/docs/midi-controller-mapping.md` | New section, "Using the visual mapper". |

Build steps: [../docs/BUILD-WINDOWS.md](../docs/BUILD-WINDOWS.md). Design:
[../docs/DESIGN.md](../docs/DESIGN.md).

Like AetherSDR, this patch is GPL-3.0.
