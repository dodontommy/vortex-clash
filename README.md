# Vortex Clash

2D fighting game prototype with deterministic simulation and rollback-ready architecture.

## macOS Build and Run

### 1) Install dependencies

```bash
brew install cmake raylib
```

### 2) Configure

From the repository root:

```bash
cmake -B build -DVORTEX_BUILD_GAME=ON -DVORTEX_ALLOW_NET_FETCH=ON
```

### 3) Build

```bash
cmake --build build -j
```

### 4) Run

```bash
./build/vortex_clash
```

## Troubleshooting

- If you only see `test_states` in `build/`, the game target likely failed to compile. Re-run:

```bash
cmake --build build -j
```

- If you get raylib-related compile/link errors, make sure Homebrew raylib is installed and visible to CMake:

```bash
brew reinstall raylib
cmake -B build -DVORTEX_BUILD_GAME=ON -DVORTEX_ALLOW_NET_FETCH=ON
```

## Feel Audit Tools

- Enable deterministic combat trace:

```bash
VORTEX_TRACE=1 ./build/vortex_clash
```

- Split a gameplay clip into 60fps frames + contact sheet:

```bash
./tools/feel_audit_frames.sh captures/session01.mp4
```

- Generate a trace-based feel report:

```bash
./tools/feel_trace_report.py --trace combat_trace.csv --out captures/session01_audit.md
```

- One-command audit pipeline (frames + report):

```bash
./tools/run_feel_audit.sh captures/session01.mp4
```

See [docs/FEEL_AUDIT.md](docs/FEEL_AUDIT.md) for full workflow.
