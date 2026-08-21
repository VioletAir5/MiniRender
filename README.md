# RenderLab

RenderLab is a beginner-friendly real-time rendering workbench built with C++20, Qt 6, and OpenGL.
It is designed to grow toward a backend-independent render graph and a Vulkan renderer while keeping
the rendering pipeline inspectable for graphics students.

## Prerequisites

- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.25+
- vcpkg at `D:/vcpkg`

The checked-in preset pins vcpkg to the Visual Studio 2022 installation at
`D:/tool/visual studio 2022` so all dependencies and project targets use the same MSVC ABI.

## Configure, build, and test

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Run the editor:

```powershell
./build/windows-msvc2022/Debug/RenderLab.exe
```

## Initial scope

- Dockable Qt editor shell
- OpenGL 3.3 core-profile viewport
- Versioned CMake/vcpkg dependency manifest
- fastgltf-based glTF dependency ready for the asset pipeline
- Unit-test target
- Layered architecture ready for SceneDocument, AssetManager, RenderGraph, and Vulkan
