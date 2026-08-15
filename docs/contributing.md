# Contributing

## Configuring a local release build

Shared build settings live in `../CMakePresets.json`. Keep paths and settings that
are specific to your machine in `../CMakeUserPresets.json`, which is ignored by
Git.

For example, create `../CMakeUserPresets.json` at the repository root with a local
release preset that inherits the shared `release` configuration:

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

Configure and build it from the command line with:

```powershell
cmake --preset local-release
cmake --build build/local-release
```

In CLion, enable CMake Presets integration and select `local-release` as the
CMake profile.
