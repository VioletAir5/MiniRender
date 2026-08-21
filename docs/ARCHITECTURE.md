# RenderLab architecture

RenderLab is split into editor, scene, asset, renderer, and backend layers. The UI must not own GPU
resources, and renderer code must not depend on editor widgets.

```text
Qt Editor UI -> Commands -> SceneDocument -> Scene Extractor -> RenderWorld
                              AssetManager --------------------^       |
                                                                      v
                                                               RenderGraph
                                                                      |
                                                               IRenderDevice
                                                               /           \
                                                          OpenGL          Vulkan
```

The initial milestone provides the editor shell and an OpenGL viewport. SceneDocument, asset import,
undo/redo, and RenderGraph are the next implementation milestones.

