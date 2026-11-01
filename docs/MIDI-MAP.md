# MP3 e2 MIDI map and starter layout

What every control on the Hercules DJControl MP3 e2 (also MP3 LE and Glow) sends, and what the starter profile `profiles/Hercules MP3 e2.xml` binds it to in AetherSDR.

MIDI numbers come from the [Mixxx](https://github.com/mixxxdj/mixxx) MP3 e2 mapping. The controller's factory default is MIDI channel 1; the profile listens on any channel. Notes send 127 on press and 0 on release. The two jog wheels and two pitch knobs are endless: they send 1 for a step right and 127 for a step left.

The **Loops/Fx** button sends nothing itself. It switches the four number keys of its deck between the notes for keys 1–4 and keys 5–8, so each deck has eight number keys.

## Deck A

| Control | Type | MIDI | Starter profile |
|---|---|---|---|
| Deck A key 1 | button | Note 0x01 (1) | Mode Down |
| Deck A key 5 | button | Note 0x05 (5) | NR2 (Spectral) |
| Deck A key 2 | button | Note 0x02 (2) | Mode Up |
| Deck A key 6 | button | Note 0x06 (6) | RN2 (RNNoise) |
| Deck A key 3 | button | Note 0x03 (3) | Squelch On/Off |
| Deck A key 7 | button | Note 0x07 (7) | Band Zoom |
| Deck A key 4 | button | Note 0x04 (4) | TNF On/Off |
| Deck A key 8 | button | Note 0x08 (8) | Segment Zoom |
| Deck A Master Tempo | button | Note 0x13 (19) | Noise Blanker |
| Deck A Pitch knob | endless wheel | CC 0x32 (50) | — |
| Deck A Pitch bend − | button | Note 0x0A (10) | Filter Narrow |
| Deck A Pitch bend + | button | Note 0x0B (11) | Filter Widen |
| Deck A Sync | button | Note 0x12 (18) | Noise Reduction |
| Deck A Rewind | button | Note 0x0C (12) | Tune Step Down |
| Deck A Fast forward | button | Note 0x0D (13) | Tune Step Up |
| Deck A Jog wheel | endless wheel | CC 0x30 (48) | VFO Tune Knob |
| Deck A Cue | button | Note 0x0E (14) | Mode LSB |
| Deck A Play | button | Note 0x0F (15) | Mode USB |

## Mixer and centre

| Control | Type | MIDI | Starter profile |
|---|---|---|---|
| Deck A Treble knob | knob / fader | CC 0x35 (53) | Squelch Level |
| Deck A Medium knob | knob / fader | CC 0x36 (54) | AGC Threshold |
| Deck A Bass knob | knob / fader | CC 0x37 (55) | Audio Pan |
| Listen A (headphone) | button | Note 0x10 (16) | Audio Mute |
| Load A | button | Note 0x11 (17) | Previous Slice |
| Deck A Volume fader | knob / fader | CC 0x34 (52) | AF Gain |
| Scratch | button | Note 0x2D (45) | Tune Lock |
| Automix | button | Note 0x2E (46) | VOX |
| Up arrow | button | Note 0x29 (41) | Band Up |
| Folder | button | Note 0x2C (44) | NR Cycle |
| Files | button | Note 0x2B (43) | AGC Cycle |
| Down arrow | button | Note 0x2A (42) | Band Down |
| Crossfader | knob / fader | CC 0x38 (56) | Master Volume |
| Deck B Treble knob | knob / fader | CC 0x3A (58) | Mic Level |
| Deck B Medium knob | knob / fader | CC 0x3B (59) | Monitor Volume |
| Deck B Bass knob | knob / fader | CC 0x3C (60) | CW Speed (WPM) |
| Listen B (headphone) | button | Note 0x24 (36) | Monitor |
| Load B | button | Note 0x25 (37) | Next Slice |
| Deck B Volume fader | knob / fader | CC 0x39 (57) | RF Power |

## Deck B

| Control | Type | MIDI | Starter profile |
|---|---|---|---|
| Deck B key 1 | button | Note 0x15 (21) | Mode CW |
| Deck B key 5 | button | Note 0x19 (25) | Auto Notch |
| Deck B key 2 | button | Note 0x16 (22) | Mode AM |
| Deck B key 6 | button | Note 0x1A (26) | RIT On/Off |
| Deck B key 3 | button | Note 0x17 (23) | Mode FM |
| Deck B key 7 | button | Note 0x1B (27) | XIT On/Off |
| Deck B key 4 | button | Note 0x18 (24) | Mode DIGU |
| Deck B key 8 | button | Note 0x1C (28) | Split |
| Deck B Master Tempo | button | Note 0x27 (39) | ATU Start |
| Deck B Pitch knob | endless wheel | CC 0x33 (51) | — |
| Deck B Pitch bend − | button | Note 0x1E (30) | Pan Zoom Out |
| Deck B Pitch bend + | button | Note 0x1F (31) | Pan Zoom In |
| Deck B Sync | button | Note 0x26 (38) | Speech Processor |
| Deck B Rewind | button | Note 0x20 (32) | Tune Down 1 MHz |
| Deck B Fast forward | button | Note 0x21 (33) | Tune Up 1 MHz |
| Deck B Jog wheel | endless wheel | CC 0x31 (49) | — |
| Deck B Cue | button | Note 0x22 (34) | TUNE |
| Deck B Play | button | Note 0x23 (35) | MOX |

Unbound in the starter profile: Deck A and Deck B pitch knobs and the Deck B jog wheel. AetherSDR drives only VFO tuning from an endless wheel, and only one control can hold it, so those three stay free unless you move the VFO to one of them.
