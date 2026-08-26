# RenderLab architecture

RenderLab separates scene extraction, rendering policy, graphics backends, and window-system
surfaces. API-specific objects must not cross a backend or surface boundary.

```text
Qt Editor UI
     |
SceneDocument
     |
SceneRenderer (GLM only)
     |
RenderFrame / RenderItem
     |
IRenderBackend
     |
OpenGLBackend ---- OpenGLMesh / OpenGLShaderProgram

RenderViewport (API-neutral QWidget container)
     |
IRenderSurface
     |
OpenGLRenderSurface (QOpenGLWidget lifecycle)
```

## Dependency rules

- `scene/`, `RenderFrame`, and `SceneRenderer` may not include Qt or graphics API headers.
- `SceneRenderer` extracts immutable per-frame data; it does not issue draw calls.
- `IRenderBackend` receives a `RenderFrame` and owns GPU rendering policy and resources.
- OpenGL symbols and `QOpenGL*` resource wrappers belong under `backends/opengl/`.
- API-specific presentation and context lifecycle belong under `surfaces/`.
- Editor widgets communicate with rendering through `RenderViewport` and `IRenderSurface`.
- Components store asset IDs, never native GPU handles.

A future Vulkan implementation adds `VulkanBackend` and `VulkanRenderSurface`. SceneDocument,
SceneRenderer, RenderFrame, and editor feature code remain unchanged. A lower-level command-list RHI
and RenderGraph can be introduced when multiple render passes require them.

