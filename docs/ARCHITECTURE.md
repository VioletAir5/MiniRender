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
     |-- RenderPipeline ---- ForwardPass / OutlinePass / GridPass
     |
RenderFrame / RenderItem
     |
IRenderSurface::setFrame
     |
OpenGLRenderSurface (QOpenGLWidget lifecycle)
     |
RenderPipeline ---- IRenderBackend / IRenderCommandList (API-neutral RHI)
     |
OpenGLBackend ---- OpenGLCommandList ---- OpenGLMesh / OpenGLShaderProgram
```

## Dependency rules

- `EditorCamera` owns navigation state and produces an API-neutral `RenderView`.
- A `RenderView` carries view/projection matrices without Qt or native graphics handles.
- `RenderViewport` owns editor-camera state and frame orchestration; it keeps only a non-owning
  scene reference.
- `IRenderSurface` receives completed frames and never accesses scene or editor state.
- `scene/`, `RenderFrame`, and `SceneRenderer` may not include Qt or graphics API headers.
- `SceneRenderer` extracts immutable per-frame data; it does not issue draw calls.
- `RenderPipeline` owns the rendering policy and executes an ordered, non-owning list of
  API-neutral `IRenderPass` objects.
- Each pass exposes a stable name and an enabled flag. `RenderPipeline::setRenderPassEnabled`
  can toggle optional passes without exposing backend objects or changing pass order.
- Passes may depend on `RenderFrame`, `RenderPassContext`, and the RHI interfaces, but may not
  include Qt, GLAD, or backend headers.
- `IRenderBackend` owns frame-level GPU resources and exposes an `IRenderCommandList`; it does
  not own or schedule rendering passes.
- `IRenderCommandList` is the API-neutral boundary for state changes and draw requests.
- `backends/opengl/` uses GLAD and raw `GLuint`/`gl*` calls; Qt headers are forbidden there.
- `OpenGLRenderSurface` is the only Qt/OpenGL bridge: it owns the context lifecycle, restores
  the `QOpenGLWidget` framebuffer, and presents the latest submitted frame. It does not own a
  scene, camera, or `SceneRenderer`.
- API-specific presentation belongs under `surfaces/`; GPU resources belong to the backend.
- Editor widgets communicate with rendering through `RenderViewport` and `IRenderSurface`.
- Components store asset IDs, never native GPU handles.

A future Vulkan implementation adds `VulkanBackend`, `VulkanCommandList`, and
`VulkanRenderSurface`. SceneDocument, SceneRenderer, RenderFrame, RenderPipeline, passes, and editor
feature code remain unchanged. A RenderGraph can be introduced later if pass resource dependencies
need automatic scheduling and lifetime analysis.

