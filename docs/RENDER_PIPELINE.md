# Render pipeline

RenderLab keeps pipeline orchestration independent from the graphics API:

- `RenderPipelineDescriptor` is API-neutral configuration.
- `RenderPassFactory` maps stable pass type IDs to one backend implementation.
- `RenderPipeline` owns lifecycle, resize order, execution order, and rollback.
- `RenderGraphCompiler` validates logical resources and produces a stable
  topological execution order.
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

Passes can now declare explicit dependencies plus logical resource reads and
writes. Transient resources require exactly one producer, unknown resources and
cycles fail during pipeline construction, and independent passes keep their
declaration order.

The next resource layer will consume the compiled declarations:

- `OpenGLRenderResources` allocates concrete textures and framebuffer
  combinations from the API-neutral descriptors;
- the pipeline executes the compiled pass order.

Asset textures and transient graph textures must use different handle types.

## Material shaders

`ShaderLibrary` is owned above the graphics backend. Materials reference its
API-neutral `ShaderHandle`, while `OpenGLShaderCache` lazily compiles the
corresponding source bundle. An invalid or missing material shader falls back to
the built-in PBR shader. Other backends can cache their own program or pipeline
objects for the same handle.

To use a custom surface shader, register its relative vertex/fragment paths in
`ShaderLibrary` and assign the returned handle to `MaterialAsset::shader`.
Surface shaders currently share the PBR vertex layout and named uniform
contract; uniforms absent from a simpler custom shader are safely ignored.

## Lighting and shadows

`SceneRenderer` extracts an API-neutral list of up to eight Directional,
Point, or Spot lights. Shadow-casting meshes are emitted into a separate queue.
The default graph runs `Directional Shadow` before `Forward`; the shadow pass
writes a fixed-size depth texture and Forward samples it with the technique
selected on `LightComponent`:

- `None` disables shadowing.
- `Hard` performs one depth comparison.
- `Pcf` performs a 3 by 3 percentage-closer filter.

The first enabled shadow-casting directional light owns the current shadow map.
Point/Spot shadow maps, cascades, and alpha-masked shadow casters remain separate
future passes; the multi-light forward shading path already supports their
direct illumination.
