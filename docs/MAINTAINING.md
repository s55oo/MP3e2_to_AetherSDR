# Maintaining this repository

## Changing the starter profile

There are two ways:

- **Starting from scratch:** open `web-mapper/index.html`, change the layout, click **Copy
  profile XML**, and paste the text over `profiles/Hercules MP3 e2.xml`.
- **Starting from your AetherSDR setup:** in AetherSDR, use **Export…** in the MIDI Mapping
  dialog. Then paste the exported file under **Edit an existing profile** in the web mapper,
  check it, and copy it out.

Then update the table in [MIDI-MAP.md](MIDI-MAP.md) and the summary in the
[README](../README.md) to match.

The web mapper's **Load ham starter layout** button uses its own copy of the layout: the
`PRESET` block in `web-mapper/index.html`. Change it there too, so the button and the profile
file agree.

## Updating the Visual Mapper patch

The patch lives in AetherSDR's own source tree. To change it:

```bat
cd AetherSDR
git checkout -b visual-midi-mapper <commit in aethersdr-patch\BASE_COMMIT.txt>
git am ..\MP3e2_to_AetherSDR\aethersdr-patch\0001-MIDI-visual-mapper.patch
:: edit, build, test, then fold the edits into the one commit:
git commit -a --amend
```

Copy the result back into this repository:

```bat
git format-patch -1 --stdout HEAD > ..\MP3e2_to_AetherSDR\aethersdr-patch\0001-MIDI-visual-mapper.patch
```

Also copy the changed files into `aethersdr-patch\source\`, keeping the folder structure.

AetherSDR changes almost daily. To move the patch onto a newer AetherSDR, rebase the branch
onto the newer commit, fix any conflicts, and update `BASE_COMMIT.txt`.

## Adding another controller

A controller is one JSON layout file. The format is in `src/core/MidiControllerLayout.h`
(see `aethersdr-patch/source/`), and the MP3 e2 file is a full example.

To try a new layout, drop it into AetherSDR's `midi\layouts` folder:

- Windows: `%LOCALAPPDATA%\AetherSDR\midi\layouts\`
- Linux: `~/.config/AetherSDR/midi/layouts/`
- macOS: `~/Library/Preferences/AetherSDR/midi/layouts/`

To ship a layout with the patch, add the file under `resources/midi-layouts/` and list it in
`resources/resources.qrc`.

## Releases

Record each change in [CHANGELOG.md](../CHANGELOG.md). Tag releases `vX.Y.Z` so others can
point at a known-good version.
