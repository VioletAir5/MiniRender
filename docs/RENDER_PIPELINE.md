# Render pipeline

RenderLab keeps pipeline orchestration independent from the graphics API:

- `RenderPipelineDescriptor` is API-neutral configuration.
- `RenderPassFactory` maps stable pass type IDs to one backend implementation.
- `RenderPipeline` owns lifecycle, resize order, execution order, and rollback.
- `IRenderPass` contains no OpenGL, Vulkan, or Qt types.
- Classes under `backends/opengl/passes` implement the current OpenGL backend.

The default editor descriptor lives in
`src/renderer/pipeline/BuiltInRenderPipeline.h` and the render surface supplies
it to the backend. OpenGL registrations are isolated in
`OpenGLPassRegistry.cpp`; `OpenGLBackend` does not include concrete pass types.
A future Vulkan backend can register Vulkan implementations for the same stable
type IDs and consume that same descriptor.

## Add a pass

1. Define a stable type ID in `BuiltInRenderPipeline.h`.
2. Add a descriptor entry at the required position.
3. Implement `IRenderPass` under the backend's `passes` directory.
4. Register a creator in that backend's pass registry.
5. Add CPU-only lifecycle tests when the generic pipeline behavior changes.

Passes must set every graphics state they depend on and release their resources
from `shutdown()`. Optional passes may fail initialization without disabling the
whole renderer; required passes trigger reverse-order rollback.

## Next architectural layer

This pipeline intentionally orders passes but does not yet allocate transient
textures or infer dependencies. When shadow maps and post-processing introduce
real intermediate resources, add a render graph above the same `IRenderPass`
lifecycle:

- passes declare resource reads and writes;
- a compiler validates dependencies and sorts nodes;
- the backend allocates concrete textures and framebuffers;
- the pipeline executes the compiled pass order.

Asset textures and transient graph textures must use different handle types.
