# Architecture

## Stack
- **Language:** C99
- **Graphics/Input/Audio:** Raylib 5.0
- **Netcode:** GGPO (rollback) planned; core simulation is being prepared for integration
- **Build:** CMake with FetchContent

## Key Decisions
- Fixed-point math (16.16) for determinism — no floats in simulation
- Flat, pointer-free GameState struct for fast GGPO serialization
- Data-driven character definitions (per-character .c files with MoveData arrays)
- Rendering completely separated from simulation
- Audio is fire-and-forget, not part of rollback state

## Project Structure
See PLAN.md Section 7.1 for full directory layout.
