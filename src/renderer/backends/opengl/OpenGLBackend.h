#pragma once

#include "renderer/backends/opengl/OpenGLMesh.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/rhi/IRenderBackend.h"

namespace renderlab {

class OpenGLBackend final : public IRenderBackend {
public:
    bool initialize() override;
    void shutdown() override;
    void resize(int width, int height) override;
    void render(const RenderFrame& frame) override;

private:
    OpenGLShaderProgram shader_;
    OpenGLMesh Mesh_;
    bool initialized_{false};
};

} // namespace renderlab
