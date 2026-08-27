# RenderLab architecture

RenderLab separates scene extraction, rendering policy, graphics backends, and window-system
surfaces. API-specific objects must not cross a backend or surface boundary.

```text
Qt Editor UI
     | mouse input
OpenGLRenderSurface
     |
EditorCamera (GLM only) ---- RenderView
                                  |
SceneDocument ---------------- SceneRenderer
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

- `EditorCamera` owns navigation state and produces an API-neutral `RenderView`.
- A `RenderView` carries view/projection matrices without Qt or native graphics handles.
- `scene/`, `RenderFrame`, and `SceneRenderer` may not include Qt or graphics API headers.
- `SceneRenderer` extracts immutable per-frame data; it does not issue draw calls.
- `IRenderBackend` receives a `RenderFrame` and owns GPU rendering policy and resources.
- `backends/opengl/` uses GLAD and raw `GLuint`/`gl*` calls; Qt headers are forbidden there.
- `OpenGLRenderSurface` is the only Qt/OpenGL bridge: it owns the context lifecycle and restores
  the `QOpenGLWidget` framebuffer before rendering.
- API-specific presentation belongs under `surfaces/`; GPU resources belong to the backend.
- Editor widgets communicate with rendering through `RenderViewport` and `IRenderSurface`.
- Components store asset IDs, never native GPU handles.

A future Vulkan implementation adds `VulkanBackend` and `VulkanRenderSurface`. SceneDocument,
SceneRenderer, RenderFrame, and editor feature code remain unchanged. A lower-level command-list RHI
and RenderGraph can be introduced when multiple render passes require them.

