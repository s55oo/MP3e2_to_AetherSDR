# Project log

How this project came together, in the order it happened (1 November 2026).

## 1. Starting point

The goal: control a FLEX-6400 from a Hercules DJControl MP3 e2 in AetherSDR, the way KE9NS
did for PowerSDR.

AetherSDR already has MIDI built in:

- a MIDI Mapping dialog with **Learn** and manual entry
- named profiles with **Import…** and **Export…**
- relative-encoder handling for VFO tuning

So no extra software sits between the controller and the radio. The work was to find what the
MP3 e2 sends and turn that into an AetherSDR profile.

## 2. Finding the MIDI map

The MP3 e2 is not a class-compliant MIDI device; it needs the Hercules driver. Its full MIDI
map came from the Mixxx project's MP3 e2 mapping:

- **Buttons** send notes.
- **Faders and EQ knobs** send absolute CC 0–127.
- **Jog wheels and pitch knobs** are endless and send 1 or 127 per step.

The **Loops/Fx** button sends nothing itself: it switches the four number keys between two
sets of notes, so each deck really has eight keys. The full table is in
[MIDI-MAP.md](MIDI-MAP.md).

AetherSDR's own source supplied the other half:

- the profile format (`<MidiProfile>` with one `<Binding>` per control)
- the list of mappable functions, 72 fixed ones plus modes, bands and EQ bands
- three rules that shaped the design:
  - **One control per function:** a second binding for the same function replaces the first.
  - **Endless wheels only tune:** AetherSDR decodes relative encoders only for VFO tuning,
    and only one control can hold it.
  - **No shared notes:** both mappers model a bank switch like Loops/Fx explicitly, so keys
    1–4 and 5–8 never clash.

xxFlexController (DM5XX) was looked at too. It is a separate Node.js program for the newer
Hercules Starlight that talks to the radio directly, so it wasn't needed here.

## 3. First profile and first test

A starter profile put VFO tuning on the left jog wheel, receive controls on Deck A and transmit
controls on Deck B. The first test failed. The fix was in AetherSDR, not the profile:

- **Import… only stores a profile.** You also have to pick it and click **Load**.
- **The MIDI port must be connected** with **Connect** in the same dialog.

After that the controller worked. Both steps are now in the README's quick start.

## 4. Web mapper

Next came a click-to-map page:

1. **First version:** a schematic of the controller. Click a control, pick a function from a
   list filtered to what that control can drive, copy the profile.
2. **Labels:** the function is printed above every control, so the whole layout reads at a
   glance.
3. **Photos:** **Use my photo…** shows a picture of the controller behind the labels, with
   draggable markers.
4. **Real layout:** the controls were rearranged to match the real MP3 e2, using a top-view
   photo as the reference.

## 5. Band buttons

The first starter profile put a band on each number key. That was swapped for just
**Band ▲ / Band ▼** on the centre arrow buttons. That freed the keys for mode −/+, squelch,
TNF, NR2, RN2 and zoom, and moved the slice buttons to Load A/B.

## 6. Visual Mapper inside AetherSDR

The web mapper still needs a copy, import and load step. So the same idea was built into
AetherSDR as a patch, written to the project's conventions (Qt6/C++20, theme tokens only,
validated input, atomic saves, a test and user documentation):

- **Layout model:** `MidiControllerLayout` reads a JSON description of a controller.
- **MP3 e2 layout:** a layout file for the MP3 e2 is bundled.
- **Visual Mapper window:** opened from the MIDI Mapping dialog. Changes apply live to the
  same bindings the table edits, and touching a control on the hardware selects it.
- **Test:** `midi_controller_layout_test` passes all 16 checks.

The new sources compile cleanly against Qt headers, and the patch applies cleanly to AetherSDR
at commit `330ef344`. The window itself still has to be tried in a full Windows build; see
[BUILD-WINDOWS.md](BUILD-WINDOWS.md).

## 7. Publishing to GitHub

Everything was gathered into this repository, `s55oo/MP3e2_to_AetherSDR`:

- the starter profile
- the web mapper, made to open as a standalone page
- the Visual Mapper patch with its sources
- documentation, generated from the same layout data as the patch so the two agree

The first push was refused, because Claude had no access to the repository. There were two
ways forward:

- **Push it yourself:** the repository was packed into a git bundle
  (`MP3e2_to_AetherSDR.bundle`), with four commands to clone and push it.
- **Give Claude access:** install the Claude GitHub App with access to this one repository
  only.

The second was chosen, and the push went through.

To give Claude access to a repository in future:

1. Signed in as the repository's owner, open
   <https://github.com/apps/claude/installations/select_target>.
2. Install the Claude app, choose **Only select repositories**, pick the repository and
   save.
3. If an earlier installation stops working, reconnect GitHub at
   <https://claude.ai/customize/connectors>.

How to keep this repository up to date is in [MAINTAINING.md](MAINTAINING.md).

## Next

- **Test the Visual Mapper** on Windows with the MP3 e2 connected.
- **Offer it to AetherSDR:** once it works, offer the patch to the AetherSDR authors as a pull
  request (the project requires signed commits), or suggest it through
  **Help → Submit your Idea…**.
- **Possible later work:**
  - Let one function sit on two controls. This needs a change to AetherSDR's binding store.
  - Add Learn from inside the picture.
  - Add layouts for more controllers.
