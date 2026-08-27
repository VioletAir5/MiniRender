# RenderLab architecture

RenderLab separates scene extraction, rendering policy, graphics backends, and window-system
surfaces. API-specific objects must not cross a backend or surface boundary.

```text
Qt Editor UI
     |
RenderViewport (graphics-API-neutral Qt controller)
     |-- SceneDocument* (non-owning)
     |-- EditorCamera (GLM only) ---- RenderView
     |-- SceneRenderer
     |
RenderFrame / RenderItem
     |
IRenderSurface::setFrame
     |
OpenGLRenderSurface (QOpenGLWidget lifecycle)
     |
IRenderBackend
     |
OpenGLBackend ---- OpenGLMesh / OpenGLShaderProgram
```

## Dependency rules

- `EditorCamera` owns navigation state and produces an API-neutral `RenderView`.
- A `RenderView` carries view/projection matrices without Qt or native graphics handles.
- `RenderViewport` owns editor-camera state and frame orchestration; it keeps only a non-owning
  scene reference.
- `IRenderSurface` receives completed frames and never accesses scene or editor state.
- `scene/`, `RenderFrame`, and `SceneRenderer` may not include Qt or graphics API headers.
- `SceneRenderer` extracts immutable per-frame data; it does not issue draw calls.
- `IRenderBackend` receives a `RenderFrame` and owns GPU rendering policy and resources.
- `backends/opengl/` uses GLAD and raw `GLuint`/`gl*` calls; Qt headers are forbidden there.
- `OpenGLRenderSurface` is the only Qt/OpenGL bridge: it owns the context lifecycle, restores
  the `QOpenGLWidget` framebuffer, and presents the latest submitted frame. It does not own a
  scene, camera, or `SceneRenderer`.
- API-specific presentation belongs under `surfaces/`; GPU resources belong to the backend.
- Editor widgets communicate with rendering through `RenderViewport` and `IRenderSurface`.
- Components store asset IDs, never native GPU handles.

A future Vulkan implementation adds `VulkanBackend` and `VulkanRenderSurface`. SceneDocument,
SceneRenderer, RenderFrame, and editor feature code remain unchanged. A lower-level command-list RHI
and RenderGraph can be introduced when multiple render passes require them.

