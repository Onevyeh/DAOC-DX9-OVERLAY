# DAOC-DX9-OVERLAY
DAoC HD Vegetation Proxy (Direct3D9 Hook)
Project Overview

This project is a Direct3D9 proxy DLL designed to inject high-definition vegetation models and shaders into Dark Age of Camelot (DAoC). Inspired by the Morrowind Graphics Extender XE (MGE XE) and the groundcover logic from Aesthesia Groundcover, it enables GPU-level injection of HD grass and leaves using NIF models and HLSL effects.

This proxy is built on top of the excellent open-source base from D3D9 Proxy Master.

Features

 Direct3D9 Hook: Intercepts all major D3D9 calls.

 Texture hashing system: Identifies vegetation via texture memory hashing.

 Automatic vertex buffer capture: Dumps vertex buffers based on hash detection.

 Custom mesh injection: Replaces native vegetation with high-poly models.

 Shader application: Applies dynamic wind movement to grass and leaves via custom .fx shaders.

 Log system: Generates debug logs for DrawPrimitive, SetStreamSource, shaders, etc.

Inspired By

MGE XE: For its injection techniques and shader pipeline logic.

Aesthesia Groundcover: Groundcover management and model distribution.

mw-groundcover-generator: Idea for procedural generation and buffer structure.

How It Works

Texture identification: Each texture used in a frame is hashed and compared against known vegetation/leaf hashes.

Custom models: .nif models (NiTriShape) are loaded and converted into Direct3D9 buffers.

Draw interception: If a hash matches, the proxy swaps in the HD mesh and applies the appropriate .fx shader.

DX11 Future Option (Experimental)

We are evaluating a full port to Direct3D11. Potential benefits include:

Full HLSL 5.0 support

Instanced rendering

GPU-based culling and LOD

⚠️ This is experimental and requires substantial rewriting of the current codebase. Contributors with D3D11 experience are very welcome.

Current Limitations / Bugs

❌ Multi-hash grass support is partial (only one grass model loaded).

❌ Performance profiling is pending (vertex injection may lag).

❌ Shader parameters (like wind strength) are hardcoded.

❌ Logs are saved to fixed C:\D3D9Proxy\ paths.

⚠️ Texture hashing assumes all data is readable via LockRect, which may fail on some hardware.

How to Build

Open d3dproxy.vcxproj in Visual Studio 2019 or later.

Build the DLL in Release mode.

How to Use

Replace the game's d3d9.dll with the compiled proxy DLL.

Place your .nif models in the specified directory (Data Files\Meshes\Grass\).

Set texture hashes in KnownVegetation.cpp.

Ensure your shaders are in C:\D3D9Proxy\shaders\.

Contributing

See CONTRIBUTING.md for code guidelines, PR process, and best practices.

This project is experimental and not affiliated with Mythic Entertainment or Broadsword. Use at your own risk.
