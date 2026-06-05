# RVTK

[中文文档](README_CN.md)

Realtime Visualization Toolkit for Unreal Engine.

This plugin currently provides one runtime component: `URVTKPolyDataComponent`. It loads a binary triangle mesh, loads a directory of scalar time-step files, and colors the mesh every frame from the active time step.

## Component

Add `RVTKPolyDataComponent` to an Actor to display animated scalar data on a polydata mesh.

The component is a `UDynamicMeshComponent`, so it owns the rendered mesh directly. At startup it:

1. Loads the mesh from `Poly Bin Path`.
2. Loads scalar `.bin` files from `Scalar Bin Directory`.
3. Builds the dynamic mesh.
4. Applies the plugin material.
5. Waits for time updates through the time-series API.

## Data Setup

Configure these properties on the component:

- `Poly Bin Path`: path to the mesh `.bin` file, usually named `mesh.bin`.
- `Scalar Bin Directory`: directory containing scalar `.bin` files for each time step.

Paths are stored relative to the project `Content` directory when edited in the Unreal Editor.

The scalar files are loaded from the selected directory with a `*.bin` search. File names are sorted numerically, so names such as `0.bin`, `1.bin`, `2.bin` are expected. Each scalar file must contain the same number of scalar values as the mesh has vertices.

## Time-Series API

The main runtime API is under the `Time Series` Blueprint category.

- `GetTimeRange()`: returns the first and last available time step.
- `GetDuration()`: returns `EndTime - StartTime`.
- `GetNumberOfSteps()`: returns the number of loaded scalar files.
- `GetCurrentTimeStep()`: returns the last time step applied to the mesh.
- `UpdateTimeStep(float TimeStep)`: updates mesh colors for the requested time.

RVTK does not own your clock. Your game, simulation, or Blueprint should keep its own playback time and call `UpdateTimeStep` each frame.

Example Blueprint flow:

1. Store a `PlaybackTime` float.
2. On Tick, add `Delta Seconds * PlaybackSpeed`.
3. Wrap or clamp the value using `GetTimeRange()` / `GetDuration()`.
4. Call `UpdateTimeStep(PlaybackTime)` on the `RVTKPolyDataComponent`.

If the requested time falls between two scalar files, the component interpolates scalar values before applying the color lookup table.

## Useful Settings

- `Position Scale`: scales mesh vertex positions when building the dynamic mesh.
- `Lut Range`: scalar min/max range used for coloring.
- `Lut Resolution`: number of colors in the lookup table.
- `Debug`: enables the built-in debug playback loop.
- `Debug Time Scale`: controls how fast the built-in debug loop advances.

Use `Lut Range` to control how scalar values map to colors. Values outside the range are clamped.

## Included Blueprints

The plugin includes two Blueprint assets under `Plugins/RVTK/Content`:

- `BP_DebugPolyData`: quick visual debug actor. Drag it into a level and it will draw the mesh and loop playback using the component debug mode.
- `BP_ExamplePolyData`: basic usage example showing how to drive the component from Blueprint.

Use `BP_DebugPolyData` when you only need to confirm that data loads and renders. Use `BP_ExamplePolyData` as the starting point for integrating your own clock and playback controls.

## Typical Usage

1. Enable the RVTK plugin.
2. Place your mesh `.bin` file and scalar `.bin` directory under the project `Content` folder.
3. Add an Actor to the level.
4. Add an `RVTKPolyDataComponent` to that Actor.
5. Set `Poly Bin Path` to the mesh file.
6. Set `Scalar Bin Directory` to the scalar file directory.
7. In Blueprint or C++, keep a playback clock and call `UpdateTimeStep` every frame.
8. Adjust `Lut Range`, `Position Scale`, and playback speed as needed.

## Notes

- Mesh data is converted into Unreal coordinates when loaded: the component flips Y and applies `Position Scale`.
- Normals are generated from the mesh triangles.
- The default time step spacing is currently `0.1` seconds per scalar file.
- The component updates vertex colors, not mesh topology, during playback.
- For shipping builds, configure `Additional Non-Asset Directories to Copy` in the project settings to include the `bin` folder.
- When using Lumen lighting, the polydata may appear to glow. To reduce this, adjust the color scale in `M_FEM_Unlit_HeadLight_5_4.uasset`; the default is `1.0`, and `0.03` usually looks bright enough without glowing.

## Known Issues

- The source PolyData normals are problematic and may be incorrect, but this does not affect the visual result because normals are calculated in the material using `dxdy`.
