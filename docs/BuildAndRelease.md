# Build and Release Pipeline (Windows)

This repo provides one-command PowerShell scripts in `tools/` for build, test, and packaging.

## Prerequisites
- Windows 10/11
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.28+
- PowerShell 7+ (or Windows PowerShell 5.1)
- Git (required for git-hash build stamping)

## One-command workflow

### 1) Debug build
```powershell
./tools/build_debug.ps1
```

### 2) Run tests
```powershell
./tools/run_tests.ps1 -BuildIfMissing
```

### 3) Release build + tests
```powershell
./tools/build_release.ps1
```

### 4) Package portable distribution + zip
```powershell
./tools/package_dist.ps1
```
Outputs:
- Portable folder: `dist/portable`
- Archive: `dist/EngineDemo-portable.zip`

## Deterministic clean build mode
Use `-Clean -Deterministic` on `build_debug.ps1`, `build_release.ps1`, or `package_dist.ps1`.

Example:
```powershell
./tools/build_release.ps1 -Clean -Deterministic
```

What this does:
- Deletes previous build/dist output (`-Clean`).
- Reconfigures from scratch.
- Enables deterministic compiler/linker options (`ENGINE_DETERMINISTIC_BUILD=ON`).
- Uses disconnected dependency update mode (`FETCHCONTENT_UPDATES_DISCONNECTED=ON`) to avoid dependency drift after initial fetch.

## Version stamping
Version metadata comes from `version/VERSION.txt`.

At configure time, CMake captures:
- semantic version from `version/VERSION.txt`
- git hash from `git rev-parse --short=12 HEAD`

The executable embeds a build stamp in the form:
- `<version>+<git_hash>`

Runtime behavior:
- Build stamp is logged on startup.
- Window title includes the build stamp.

## Fresh-machine bootstrap
On a clean machine, run:
```powershell
./tools/build_debug.ps1
./tools/run_tests.ps1 -BuildIfMissing
./tools/build_release.ps1
./tools/package_dist.ps1
```


## Renderer smoke test
The automated smoke test binary validates renderer startup/shutdown:
```powershell
ctest --test-dir build-debug -C Debug -R renderer_smoke_tests --output-on-failure
```
