# HelloWorld for Skyrim VR

HelloWorld is an SKSE VR plugin that reports whether the active OpenVR headset
connection is wired or wireless. It also contains the MCM Helper assets needed
to expose the notification delay in SkyUI's Mod Configuration Menu.

The MCM's Papyrus script emits `HelloWorld_SettingsChanged` when the player
changes `iMessageDelay:General`. The native plugin does not consume that event
yet, so the slider is currently the Papyrus-facing part of the integration.

## Player requirements

- [Skyrim VR](https://store.steampowered.com/app/611670/The_Elder_Scrolls_V_Skyrim_VR/)
- [SKSEVR](https://skse.silverlock.org/)
- [SkyUI VR](https://github.com/Odie/skyui-vr)
- The VR build of [MCM Helper](https://www.nexusmods.com/skyrimspecialedition/mods/53000)
- [SteamVR](https://store.steampowered.com/app/250820/SteamVR/) or a fully
  compatible OpenVR runtime

Install the release archive with a mod manager and make sure `HelloWorld.esp`
is enabled. The plugin, compiled Papyrus script, and MCM configuration are all
included in the archive.

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
SKSE/Plugins/HelloWorld.dll
```

For asset-only work, build the `HelloWorldAssets` target. More detail about the
checked-in assets and quest attachment is in the
[contributing guide](docs/contributing.md).
