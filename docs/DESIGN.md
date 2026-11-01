# Visual MIDI Mapper for AetherSDR: design

The AetherSDR change in [`aethersdr-patch/`](../aethersdr-patch/). To build it, see [BUILD-WINDOWS.md](BUILD-WINDOWS.md).

## Goal

Map a MIDI controller by clicking its controls on a picture, with every
knob, fader, wheel and button showing what it is bound to. The work happens
inside AetherSDR, with no export and import step.

## Where it lives in AetherSDR

**Settings → Controllers → MIDI Mapping → Visual Mapper…** opens a new
window beside the existing MIDI Mapping dialog.

```
┌───────────────────────────────────────────────┬──────────────────────────┐
│ Controller: [Hercules MP3 e2 ▾] [Keys 1–4][5–8] │ Deck A Jog wheel          │
│                         [Photo…] [Remove photo]│ Control Change 0x30 (48)  │
│                                               │ ┌──────────────────────┐ │
│      picture of the controller                │ │ VFO Tune Knob        │ │
│      (drawing, or your photo behind it)       │ └──────────────────────┘ │
│      the bound function written above         │ [ ] Reverse  [Unassign]  │
│      every control                            │ [search functions…]      │
│                                               │ RX                       │
│                                               │   AF Gain                │
│ [x] Select a control by touching it on the    │   …                      │
│     controller                                │                          │
│ status line                                   │ hint                     │
└───────────────────────────────────────────────┴──────────────────────────┘
```

## How it behaves

- **Selecting a control:** click it in the picture. You can also press or turn
  it on the hardware, and the mapper highlights it with a brief green ring.
  The arrow keys move between controls.
- **Assigning:** one click on a function in the list. The list offers only what
  that control can drive:
  - Buttons get on/off, press and hold functions.
  - Knobs and faders get levels.
  - Endless wheels get VFO Tune Knob, the only function AetherSDR drives from a
    relative encoder.
- **One function per control:** this is AetherSDR's own rule
  (`MidiControlManager::addBinding` removes any other binding of the function).
  If you pick a function that is already used, it moves to the selected control,
  and the status line names the control it came from.
- **Saving:** every change applies immediately and is saved to `midi.settings`.
  The MIDI Mapping table refreshes, and edits made in the table refresh the
  picture.
- **Key banks:** the MP3 e2's Loops/Fx button changes which notes the four
  number keys send (keys 1–4 or 5–8). The bank buttons at the top switch the
  picture, and so does clicking Loops/Fx in the picture or pressing **B**.
- **Photo:** **Photo…** stores a copy of your photo as
  `<profile dir>/layouts/<layout-id>-photo.png`. It is drawn behind the
  controls, which become see-through, with outlined labels on top.

## Building blocks

| Piece | File | Role |
|---|---|---|
| Layout model | `src/core/MidiControllerLayout.{h,cpp}` | Reads and validates a controller layout (JSON). A file with a bad row is refused as a whole, with the reason. |
| MP3 e2 layout | `resources/midi-layouts/hercules-djcontrol-mp3-e2.json` | 55 controls with MIDI numbers from Mixxx's MP3 e2 mapping. Positions match the 650×450 top-view photo, scaled ×2. |
| Canvas | `src/gui/MidiControllerCanvas.{h,cpp}` | Paints the controller with theme tokens only, handles clicks and hover, and draws the photo. |
| Window | `src/gui/MidiVisualMapperDialog.{h,cpp}` | The selection panel, function list, banks, photo, and following the hardware. |
| Hook | `src/gui/MidiMappingDialog.{h,cpp}` | Adds the Visual Mapper… button and keeps both views in sync. |
| Test | `tests/midi_controller_layout_test.cpp` | Checks that the MP3 e2 file loads and matches its MIDI map, and that bad files are refused. |
| User doc | `docs/user/docs/midi-controller-mapping.md` | New section, "Using the visual mapper". |

## Adding another controller

Adding a controller needs only a JSON file, no code. Put it in
`%LOCALAPPDATA%\AetherSDR\midi\layouts\` and it shows up in the
**Controller** list. The format is documented at the top of
`MidiControllerLayout.h`. Each control has:

- an id and a name
- its kind (`button`, `absolute` or `encoder`)
- its MIDI message (`note` or `cc`) and number
- its shape (`key`, `pill`, `knob`, `jog`, `fader` or `crossfader`)
- its position, and optionally a bank

## AetherSDR rules it follows

- **Theme tokens only.** No hardcoded colour literals; the colour ratchet stays
  flat.
- **Boundary input validation.** Layout files and photos are size-limited and
  checked before use.
- **Atomic writes.** The photo is saved through `QSaveFile`, and bindings
  through `MidiSettings::saveBindings`.
- **No change to MIDI decoding or the binding store.** The window only edits
  the same `MidiBinding` list the table edits.

## Not done yet

- **One function on two controls.** This would need a change to AetherSDR's
  binding store. It is a separate decision for the maintainers.
- **Dragging markers onto your own photo.** The web mapper can do this. Here,
  the shipped layout is drawn to the standard 650×450 top view of the MP3 e2,
  so a photo taken straight from above and cropped to the controller's edges
  lines up.
- **Learn mode inside the picture.** You can select a control by touching it on
  the hardware instead.
