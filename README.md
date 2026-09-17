<p align="center">
  <img width="65%" height="65%" src="Content/lucy_logo.png">
</p>

Welcome to Lucy, a high-fidelity game engine focused on performance and real-time graphics.
Lucy combines GPU-driven Vulkan renderer with hardware-accelerated [Dynamic Diffuse Global Illumination (DDGI)](https://jcgt.org/published/0008/02/01/).

Lucy is being developed by me and is in an experimental state.

### How to build
To build Lucy for Windows 10/11, first make sure that the requirements below are met and run **Generate.bat** to generate the **Lucy.slnx** solution file. Open the solution in Visual Studio and build **LucyEditor**.
Most third-party dependencies are bundled with the repository. The Vulkan SDK must be installed separately. Additionally, make sure that the installation includes Slang libraries as well.

#### Requirements
- Visual Studio 2026
- [Vulkan](https://vulkan.lunarg.com/sdk/home) SDK 1.4+
- GPU with Hardware-accelerated Ray-Tracing support

### About this project
As the author of this project, I am fascinated by game engines, or computer graphics for short. I think the main reason for this fascination lies in the difficulty of creating such a project. I've never planned this engine to be the next Unreal or Unity. It is simply my hobby project that I am the architect of. 

Lucy is technically the older sister of **OxyEngine** (https://github.com/AAstroPhysiCS/OxyEngine), which I made it with Java (OpenGL), as that was the language I felt most comfortable with. As years progressed, I turned to C++ (Vulkan), which allowed me to freely use my "powers :)" with no restriction. Additionally, C++ is a language that is more widely used than Java in terms of computer graphics. So, it was also a nice step forward for my career as well.

### Rendering Architecture
- **GPU-driven rendering** with GPU-generated draw commands, indirect compute dispatch, and indirect-count drawing, keeping visibility and draw-count decisions on the GPU.
- **Multithreaded renderer** with separate main and render threads, plus a custom job system for parallel work such as model processing.
- **Custom render graph** with multi-queue scheduling, automatic resource barriers, cross-queue dependency analysis, and timeline-semaphore synchronization.
- **Bindless rendering** with globally indexed textures and samplers, and GPU-resident scene data accessed through buffer device addresses rather than per-object descriptor rebinding.
- **Shared global vertex and index buffers** with vertex pulling, using the same geometry storage for rasterization and ray tracing. (inspired by idTech-Engine)
- **Three-stage GPU visibility pipeline** for objects, submeshes, and meshlets, combining frustum and meshlet normal-cone culling without requiring mesh or task shaders.
- **Automatic meshlet and LOD generation** using meshoptimizer, including vertex-cache, vertex-fetch, and overdraw optimization.
- **LOD selection** based on screen-space geometric error and object scale. (like UE5 Nanite)

### Lighting and Global Illumination
- **Hardware-accelerated DDGI** with multi-bounce diffuse feedback through irradiance history, without pre-baked lightmaps.
- **Adaptive probe scheduling** with a configurable update budget and camera-aware priorities based on visibility, distance, probe age, and relocation state.
- **Visibility-aware probe interpolation** using depth moments, surface orientation, and positional bias to reduce light leaking.
- **Automatic probe relocation** using frontface and backface ray hits, with bounded, smoothed movement away from problematic geometry.
- **Four cascaded variance shadow maps**, rendered through multiview with a dedicated GPU-driven shadow-culling pipeline, texel-snapped shadow cameras, and separable Gaussian filtering.
- **HDR image-based lighting** with diffuse irradiance convolution, roughness-prefiltered environment reflections, and a precomputed BRDF lookup texture. DDGI handles indirect diffuse lighting inside the probe volume, while specular environment lighting remains separate.

### Screenshots
