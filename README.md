# HelloWorld for Skyrim VR

HelloWorld is an SKSE VR plugin that reports whether the active OpenVR headset
connection is wired or wireless. It includes its own menu configuration for
exposing the notification delay through SkyUI's Mod Configuration Menu (MCM).

The MCM's Papyrus script emits `HelloWorld_SettingsChanged` when the player
changes `iMessageDelay:General`. The native plugin does not consume that event
yet, so the slider is currently the Papyrus-facing part of the integration.

## Installation and mod requirements

HelloWorld requires Skyrim VR and the following separately installed mods:

1. [SKSEVR](https://skse.silverlock.org/)
2. [SkyUI VR](https://github.com/Odie/skyui-vr)
3. The Skyrim VR build of
   [MCM Helper](https://www.nexusmods.com/skyrimspecialedition/mods/53000),
   version 1.4.0 or later. MCM Helper is a separate mod and is not bundled in
   the HelloWorld archive. Select its Skyrim VR component when installing it.

The headset notification also requires
[SteamVR](https://store.steampowered.com/app/250820/SteamVR/) or a fully
compatible OpenVR runtime.

Install the requirements above first. Then install the HelloWorld release
archive with a mod manager and make sure `HelloWorld.esp` is enabled. The
HelloWorld native plugin, compiled Papyrus script, MCM configuration, and SEQ
file are included in its archive.

If the MCM Helper native plugin is missing or too old, HelloWorld records an
error in `Documents/My Games/Skyrim VR/SKSE/HelloWorld.log`. MCM Helper writes
its own `MCMHelper.log` in the same directory when it is installed and loaded.

## Build requirements

- Windows 10 or later
- Visual Studio 2022 with the **Desktop development with C++** workload
- CMake 3.21 or later and Ninja
- [vcpkg](https://github.com/microsoft/vcpkg), with `VCPKG_ROOT` set to its
  installation directory

The normal C++ build does not require the Creation Kit. The compiled ESP and
PEX are checked in under `mod/`, and CMake only stages them for packaging.

The Creation Kit and its Papyrus compiler are authoring requirements when you
change the quest in `HelloWorld.esp` or `HelloWorldMCM.psc`. After making those
changes, replace `mod/HelloWorld.esp` and/or
`mod/Scripts/HelloWorldMCM.pex` with the new outputs before building a release.

## Building

Create `CMakeUserPresets.json` if you need machine-specific output paths; see
[the contributing guide](docs/contributing.md). Then run:

```powershell
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release --output-on-failure
```

The complete mod archive is written to:

```text
build/packages/HelloWorld-<version>-SkyrimVR.7z
```

Its installable payload contains:

```text
HelloWorld.esp
MCM/Config/HelloWorld/config.json
MCM/Config/HelloWorld/settings.ini
Scripts/HelloWorldMCM.pex
Seq/HelloWorld.seq
SKSE/Plugins/HelloWorld.dll
```

For asset-only work, build the `HelloWorldAssets` target. More detail about the
checked-in assets and quest attachment is in the
[contributing guide](docs/contributing.md).
