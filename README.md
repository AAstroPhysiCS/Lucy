# Lucy

## 22.07.2022: Dropping OpenGL support.
OpenGL is deprecated. OpenGL is completely different from other rendering APIs. Maintaining OpenGL would be more difficult than actually implementing rendering features into the engine. It just simply didn't make any sense, as Lucy will be a modern renderer, capable of rendering high quality models with GI (Ray Tracing) solutions.

I've made the decision, to support DirectX12 in the future. Currently, Lucy has a very performant Vulkan renderer, which will be much more performant in the future as I develop and optimize the renderer.