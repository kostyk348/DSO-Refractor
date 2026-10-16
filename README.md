# DSO-Refractor

[![Release](https://img.shields.io/github/v/release/kostyk348/DSO-Refractor?sort=semver&color=blue)](https://github.com/kostyk348/DSO-Refractor/releases)
[![License](https://img.shields.io/badge/license-Apache-2.0-green)](https://github.com/kostyk348/DSO-Refractor/blob/main/LICENSE)
[![Last commit](https://img.shields.io/github/last-commit/kostyk348/DSO-Refractor?branch=main)](https://github.com/kostyk348/DSO-Refractor/commits/main)

OpenGL terrain + static objects renderer for **Battlefield 1942 (Demo — Tobruk/Tunisia)**.

Loads the original `.rfa` archives from the BF1942 Demo — heightmap terrain, 415 static objects (buildings, bunkers, walls, sandbags), all textured from the original DDS files.

## Build & Run

```bash
# Requires: SDL2, GLEW, OpenGL 3.3+, X11

make bin
LD_LIBRARY_PATH=/home/lain/dsa ./dsr-bin
```

Data files (`data/*.rfa`, `bf1942_data/`) from the BF1942 Demo must be present.

## Controls

| Key | Action |
|---|---|
| WASD | Move camera (XZ plane) |
| Q / E | Move camera up / down |
| RMB (drag) | Mouse look |
| F2 | Toggle wireframe |
| R | Cycle rotation mode |
| M | Toggle mirror |
| Esc | Quit |

## Features

- BF1942 `.sm` mesh loader (v9 format, Z-up → Y-up conversion)
- `.rs` shader parser (material→texture mapping)
- DDS texture loader (DXT1/3/5) from `texture.rfa`
- Terrain heightmap sampling (`Heightmap.raw`)
- Per-object `.con` positioning, rotation, scale (415 objects)
- Multi-material mesh merging (all geometry visible)
- 136 unique DDS textures loaded

## Architecture

```
dsr/        — core library (mesh, render, terrain, rfa, etc.)
main.c      — game loop, object loading, camera
data/       — BF1942 Demo archives (standardMesh.rfa, texture.rfa, etc.)
bf1942_data/ — level configuration files
```
