# BP Transmiter

This repository contains the Transmiter firmware project.

## Repository hierarchy

```
.
├── components/             # Custom project components
├── include/                # Public header files
├── lib/                    # Private project-specific libraries
├── managed_components/     # External components managed by the build system
├── src/                    # Main source code files
├── .gitignore              # Git ignore rules
├── .gitmodules             # Submodule configuration
├── betaflight_settings.txt # Specific configuration for Betaflight configurator CLI
├── CMakeLists.txt          # Build system configuration
├── dependencies.lock       # Dependency lock file for reproducible builds
├── platformio.ini          # Core PlatformIO configuration
└── readme.md               # Project documentation```

## Setup

1. Clone the repository with submodules: and initialize them:

   ```bash
   git submodule update --init --recursive
   ```

2. Install PlatformIO if not already installed.
   - For VS Code: install the PlatformIO IDE extension.
   - Or install the PlatformIO CLI:

   ```bash
   pip install platformio
   ```

3. Enter the project directory:

   ```bash
   cd /home/nouwindd/FIT/BP/BP_Transmiter
   ```

## Build

Use PlatformIO to build the project:

```bash
platformio run
```

## Upload

To upload to the connected board:

```bash
platformio run --target upload
```

## Notes

- Ensure all submodules are present before building.
- If you change the platformio configuration, re-run `git submodule update --init --recursive` as needed.
