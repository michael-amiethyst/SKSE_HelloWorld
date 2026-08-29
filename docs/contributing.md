# Contributing

## Prerequisites

Install the build tools listed in the [README](../README.md#build-requirements).

### ESP and Papyrus Prerequisites

Install the Creation Kit and its Papyrus compiler
before editing the ESP files (like [mod/HelloWorld.esp](../mod/HelloWorld.esp)) or recompiling the Papyrus source.
Example Papyrus source at [HelloWorldMCM.psc](../scripts/Source/HelloWorldMCM.psc).

## Configuring a local release build

Shared settings live in `../CMakePresets.json`. Keep paths and settings that are
specific to your machine in `../CMakeUserPresets.json`, which is ignored by Git.

For example:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "local-release",
      "inherits": "release",
      "cacheVariables": {
        "DEPLOY_FOLDER": "C:/path/to/local/release/output"
      }
    }
  ]
}
```

Configure, build, and test it with:

```powershell
cmake --preset local-release
cmake --build build/local-release
ctest --test-dir build/local-release --output-on-failure
```

In CLion, enable CMake Presets integration and select `local-release` as the
CMake profile.

## Mod assets

The `HelloWorldAssets` CMake target owns the non-C++ parts of the package:

- `../mod/HelloWorld.esp` contains the MCM registration quest.
- `../mod/Scripts/HelloWorldMCM.pex` is the compiled Papyrus script.
- `../mod/MCM/Config/HelloWorld/config.json` describes the menu.
- `../mod/MCM/Config/HelloWorld/settings.ini` supplies the default delay.
- `../scripts/Source/HelloWorldMCM.psc` is the editable Papyrus source.

The ESP contains a start-game-enabled quest with `HelloWorldMCM`
attached. Its forced player alias has `SKI_PlayerLoadGameAlias` attached, which
lets SkyUI register the menu after a game loads.

### Generating the SEQ file

The start-game-enabled quest requires `mod/Seq/HelloWorld.seq`. CMake stops at
configuration time with a guided error when this file is absent.

Generate it with xEdit as follows:

1. Make `mod/HelloWorld.esp` visible to xEdit. If a previous build is already
   deployed through Vortex, enable `HelloWorld.esp` in Vortex's Plugins tab.
   Otherwise, temporarily copy `mod/HelloWorld.esp` to
   `SkyrimVR/Data/HelloWorld.esp`.
2. Launch `TES5VREdit.exe`. If the installation only provides `xEdit.exe`,
   launch it with the `-TES5VR` argument.
3. In the plugin selection window, click **Unselect All**, select
   `HelloWorld.esp`, and click **OK**. Required masters are selected
   automatically.
4. Wait until the Messages pane says `Background Loader: finished`.
5. In the left pane, right-click the top-level `HelloWorld.esp` entry and choose
   **Other → Create SEQ File**. Some xEdit versions label this command
   **Generate SEQ File**.
6. Confirm that the Messages pane reports one start-enabled quest file created.
   Generating the SEQ file does not modify the ESP, so no plugin changes need to
   be saved when closing xEdit.
7. Copy the generated file from
   `SkyrimVR/Data/Seq/HelloWorld.seq` to `mod/Seq/HelloWorld.seq` in this
   repository.
8. If the ESP was copied into `SkyrimVR/Data` manually in step 1, remove that
   temporary unmanaged copy before installing the packaged mod through Vortex.
9. Commit the SEQ file and rebuild the package. Regenerate it whenever a
   start-game-enabled quest is added to or removed from `HelloWorld.esp`.

See xEdit's
[Start-Game Enabled Quest documentation](https://tes5edit.github.io/docs/9-mod-utilities.html#s_9-7)
for its description of the same operation.

When the quest changes, save the updated plugin as `../mod/HelloWorld.esp`.
When the Papyrus source changes, compile it with the Creation Kit's Papyrus
compiler and replace `../mod/Scripts/HelloWorldMCM.pex`. Commit those compiled
outputs so contributors and CI can build the package without the Creation Kit.

Build only these assets with:

```powershell
cmake --build build/local-release --target HelloWorldAssets
```

This target copies the `mod` data tree to `build/ModContents`. The full plugin
build then stages the DLL there and creates the versioned archive under
`build/packages` (or `DEPLOY_FOLDER`). To build the installable archive without
building the test target, run:

```powershell
cmake --build build/local-release --target HelloWorldPackage
```

## Versioning

The `project(... VERSION ...)` value in `CMakeLists.txt` names release archives.
The manifest version in `vcpkg.json` describes this project's vcpkg manifest.
They are technically independent, but keep them equal for project releases so
build metadata does not report conflicting versions.
