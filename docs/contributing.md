# Contributing

## Prerequisites

Install the build tools listed in the [README](../README.md#build-requirements).
In particular, Spriggit needs a .NET **SDK**, not only a .NET runtime. No
Creation Kit installation and no non-VR Skyrim installation are needed.

The first `HelloWorldAssets` build downloads the following pinned inputs into
the active CMake build directory:

- Spriggit and its `Spriggit.YAML.Skyrim` serializer
- Caprica
- Skyrim Papyrus source stubs
- MCM Helper's public Papyrus SDK sources

The archives are verified with SHA-256 before extraction. Keep network access
available for the first build of a new build directory. If a download is
interrupted or its hash check fails, delete only the named archive from
`<binary-dir>/asset-tools/downloads` and rebuild.

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

If CMake selects a `dotnet.exe` that has no SDK, install .NET 9 or set the
`DOTNET_EXECUTABLE` cache variable to the SDK's executable.

## Creation-Kit-free mod assets

The `HelloWorldAssets` CMake target owns the non-C++ parts of the package:

- `../plugin/HelloWorld` is the reviewable Spriggit representation of
  `HelloWorld.esp`.
- `../scripts/Source/HelloWorldMCM.psc` is compiled to
  `Scripts/HelloWorldMCM.pex`.
- `../mcm/Config/HelloWorld/config.json` describes the menu.
- `../mcm/Config/HelloWorld/settings.ini` supplies the default delay.

The generated ESP contains a start-game-enabled quest with `HelloWorldMCM`
attached. Its forced player alias has `SKI_PlayerLoadGameAlias` attached, which
lets SkyUI register the menu after a game loads. Edit the checked-in YAML when
that quest wiring changes; do not commit the generated ESP or PEX.

Build only these assets with:

```powershell
cmake --build build/local-release --target HelloWorldAssets
```

The full plugin build stages the assets and DLL under `build/ModContents`, then
creates the versioned archive under `build/packages` (or `DEPLOY_FOLDER`).

## Versioning

The `project(... VERSION ...)` value in `CMakeLists.txt` names release archives.
The manifest version in `vcpkg.json` describes this project's vcpkg manifest.
They are technically independent, but keep them equal for project releases so
build metadata does not report conflicting versions.
