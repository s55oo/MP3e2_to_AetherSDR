# Build AetherSDR with the Visual Mapper on Windows

This builds your own copy of AetherSDR with the Visual Mapper added. Your
installed AetherSDR is not touched. Your settings and MIDI bindings are shared
between the two, so your current profile shows up in both.

## You need

- **Visual Studio 2022, version 17.14 or newer.** The free Build Tools or
  Community edition works. Install it with the "Desktop development with C++"
  workload.
- **Git, CMake 3.25 or newer, Ninja, Python 3 and 7-Zip.**
- **About 10 GB of free disk space.** Qt alone is about 2 GB.

## Steps

Open **x64 Native Tools Command Prompt for VS 2022** from the Start menu.
Then run:

```bat
cd C:\HAM
git clone https://github.com/s55oo/MP3e2_to_AetherSDR.git
git clone https://github.com/aethersdr/AetherSDR.git
cd AetherSDR

:: apply the Visual Mapper change
git apply ..\MP3e2_to_AetherSDR\aethersdr-patch\0001-MIDI-visual-mapper.patch

:: one-time setup: Qt 6.12 and FFTW (re-running is harmless)
powershell -ExecutionPolicy Bypass -File scripts\setup\setup-qt.ps1
powershell -ExecutionPolicy Bypass -File scripts\setup\setup-fftw.ps1

:: configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target AetherSDR
```

The program is `build\AetherSDR.exe`. Close your installed AetherSDR before
starting it.

## If `git apply` refuses the patch

AetherSDR changes almost daily, so the patch can stop applying cleanly.
`aethersdr-patch\BASE_COMMIT.txt` names the commit the patch was made
against. You can fix it either of two ways:

- **Build against that commit:** run `git checkout <that commit>` before
  `git apply`.
- **Copy the changed files instead:** copy everything under
  `aethersdr-patch\source\` over the clone, keeping the folder structure.

## Try it

1. Connect the MP3 e2 and start `build\AetherSDR.exe`.
2. Go to **Settings → Controllers → MIDI Mapping** and **Connect** the
   Hercules.
3. Click **Visual Mapper…**.
4. Optionally click **Photo…** and choose a photo of your controller, taken
   straight from above and cropped to its edges. It sits behind the labels.
5. Click a control, or touch it on the controller, then click a function in
   the list.

## Running the new test (optional)

```bat
cmake --build build --target midi_controller_layout_test
build\midi_controller_layout_test.exe
```

Every line should say `[ OK ]`.
