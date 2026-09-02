cmake_minimum_required(VERSION 3.25)

project(RenderLab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(RENDERLAB_BUILD_TESTS "Build RenderLab tests" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(fastgltf CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(RenderLabCore STATIC
    src/scene/EntitySnapshot.cpp
    src/scene/EntitySnapshot.h
    src/editor/EditorCamera.cpp
    src/editor/EditorCamera.h
    src/editor/ScenePicker.cpp
    src/editor/ScenePicker.h
    src/editor/TranslateGizmo.cpp
    src/editor/TranslateGizmo.h
    src/renderer/RenderView.h
    src/core/AppInfo.cpp
    src/core/AppInfo.h
    src/renderer/RenderFrame.h
    src/renderer/SceneRenderer.cpp
    src/renderer/SceneRenderer.h
    src/renderer/ShaderLibrary.cpp
    src/renderer/ShaderLibrary.h
    src/assets/ShaderAsset.h
    src/scene/Components.h
    src/scene/Entity.h
    src/scene/EntityId.h
    src/scene/SceneDocument.cpp
    src/scene/SceneDocument.h
    src/scene/TransformUtils.cpp
    src/scene/TransformUtils.h
    src/importers/GltfImporter.cpp
    src/importers/GltfImporter.h
    src/assets/AssetId.h
    src/assets/AssetHandle.h
    src/assets/AssetRegistry.cpp
    src/assets/AssetRegistry.h
    src/assets/TextureAsset.h
    src/assets/MaterialAsset.h
    src/assets/MeshAsset.h
    src/assets/MeshTangents.cpp
    src/assets/MeshTangents.h
    src/assets/ProceduralMeshLibrary.cpp
    src/assets/ProceduralMeshLibrary.h
    src/assets/PrimitiveFactory.h
    src/assets/PrimitiveFactory.cpp
    src/serialization/SceneSerializer.cpp
    src/serialization/SceneSerializer.h
)

target_include_directories(RenderLabCore
    PUBLIC src
    PRIVATE ${Stb_INCLUDE_DIR}
)
target_link_libraries(RenderLabCore PUBLIC
    fastgltf::fastgltf
    glm::glm
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# 编辑器逻辑单独成库，应用和测试共享同一份实现，避免重复编译源文件。
add_library(RenderLabEditor STATIC
    src/editor/EntityCommands.cpp
    src/editor/EntityCommands.h
    src/app/TransformInspector.cpp
    src/app/TransformInspector.h
    src/editor/TransformCommand.cpp
    src/editor/TransformCommand.h
)
target_include_directories(RenderLabEditor PUBLIC src)
target_link_libraries(RenderLabEditor PUBLIC
    RenderLabCore
    Qt6::Widgets
)

qt_add_executable(RenderLab
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/renderer/backends/opengl/OpenGLBackend.cpp
    src/renderer/backends/opengl/OpenGLBackend.h
    src/renderer/backends/opengl/OpenGLGridRenderer.cpp
    src/renderer/backends/opengl/OpenGLGridRenderer.h
    src/renderer/backends/opengl/OpenGLMesh.cpp
    src/renderer/backends/opengl/OpenGLMesh.h
    src/renderer/backends/opengl/OpenGLMeshCache.cpp
    src/renderer/backends/opengl/OpenGLMeshCache.h
    src/renderer/backends/opengl/OpenGLTextureCache.cpp
    src/renderer/backends/opengl/OpenGLTextureCache.h
    src/renderer/backends/opengl/OpenGLTexture.cpp
    src/renderer/backends/opengl/OpenGLTexture.h
    src/renderer/backends/opengl/passes/OpenGLPassContext.h
    src/renderer/backends/opengl/passes/OpenGLSceneDraw.cpp
    src/renderer/backends/opengl/passes/OpenGLSceneDraw.h
    src/renderer/backends/opengl/passes/OpenGLForwardPass.cpp
    src/renderer/backends/opengl/passes/OpenGLForwardPass.h
    src/renderer/backends/opengl/passes/OpenGLOutlinePass.cpp
    src/renderer/backends/opengl/passes/OpenGLOutlinePass.h
    src/renderer/backends/opengl/passes/OpenGLGridPass.cpp
    src/renderer/backends/opengl/passes/OpenGLGridPass.h
    src/renderer/backends/opengl/OpenGLShaderProgram.cpp
    src/renderer/backends/opengl/OpenGLShaderProgram.h
    src/renderer/rhi/IRenderBackend.h
    src/renderer/RenderViewport.cpp
    src/renderer/RenderViewport.h
    src/renderer/TransformGizmoOverlay.cpp
    src/renderer/TransformGizmoOverlay.h
    src/renderer/surfaces/IRenderSurface.h
    src/renderer/surfaces/OpenGLRenderSurface.cpp
    src/renderer/surfaces/OpenGLRenderSurface.h
)

target_link_libraries(RenderLab PRIVATE
    RenderLabCore
    RenderLabEditor
    glad::glad
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

if(MSVC)
    target_compile_options(RenderLabCore PRIVATE /W4 /permissive-)
    target_compile_options(RenderLabEditor PRIVATE /W4 /permissive-)
    target_compile_options(RenderLab PRIVATE /W4 /permissive-)
else()
    target_compile_options(RenderLabCore PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(RenderLabEditor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(RenderLab PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    set_target_properties(RenderLab PROPERTIES WIN32_EXECUTABLE TRUE)

    add_custom_command(TARGET RenderLab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:RenderLab>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
            "$<TARGET_FILE_DIR:RenderLab>/platforms/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
            "$<TARGET_FILE_DIR:RenderLab>/shaders"
        COMMENT "Deploying the Qt Windows platform plugin"
    )
endif()

install(TARGETS RenderLab RUNTIME DESTINATION bin)
install(DIRECTORY assets/shaders DESTINATION bin)

if(RENDERLAB_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 CONFIG REQUIRED)
    add_executable(RenderLabTests
        tests/AppInfoTests.cpp
        tests/AssetRegistryTests.cpp
        tests/EditorCameraTests.cpp
        tests/ScenePickerTests.cpp
        tests/SceneRendererTests.cpp
        tests/SceneDocumentTests.cpp
        tests/EntitySnapshotTests.cpp
        tests/EntityCommandTests.cpp
        tests/SceneSerializerTests.cpp
        tests/TranslateGizmoTests.cpp
        tests/TransformInspectorTests.cpp
        tests/TransformCommandTests.cpp
        tests/PrimitiveFactoryTests.cpp
        tests/ProceduralMeshLibraryTests.cpp
        tests/GltfImporterTests.cpp
        tests/ShaderLibraryTests.cpp
    )
    target_link_libraries(RenderLabTests PRIVATE
        RenderLabCore
        RenderLabEditor
        Catch2::Catch2WithMain
    )
    include(Catch)
    catch_discover_tests(RenderLabTests)
endif()
