# RenderLab

RenderLab is a beginner-friendly real-time rendering workbench built with C++20, Qt 6, and OpenGL.
Its scene extraction and editor layers are graphics-API independent so the project can grow toward
additional backends such as Vulkan while keeping the rendering pipeline inspectable.

## Prerequisites

- Visual Studio 2026 with the Desktop development with C++ workload
- CMake 3.25+
- vcpkg at `D:/vcpkg`
- Visual Studio installed at `D:/tool/visual studio` for the checked-in preset

## Configure, build, and test

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Run the editor:

```powershell
./build/windows-msvc2026/Debug/RenderLab.exe
```

## Viewport camera controls

- Middle mouse drag: orbit around the current target
- Shift + middle mouse drag: pan in the camera plane
- Mouse wheel: zoom toward or away from the target

The editor camera is independent from scene camera entities and from the OpenGL backend.

## Current scope

- Dockable Qt editor shell backed by SceneDocument
- API-neutral SceneRenderer producing RenderFrame data with GLM
- RenderViewport owns editor camera state and RenderFrame orchestration
- IRenderBackend and IRenderSurface extension boundaries
- Raw OpenGL 3.3 backend using GLAD, `GLuint`, and direct `gl*` calls
- Isolated QOpenGLWidget surface used only for context lifecycle and presentation
- Built-in indexed cube rendering with depth testing
- API-neutral orbit editor camera producing a reusable RenderView
- fastgltf dependency ready for the glTF asset pipeline
- Unit tests for scene ownership, hierarchy, and render-frame extraction

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for dependency rules.

## UML diagrams

The project includes a clang-uml preset and diagram configuration. See
[docs/CLANG_UML.md](docs/CLANG_UML.md) for Windows setup and generation commands.
