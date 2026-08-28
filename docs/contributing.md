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
