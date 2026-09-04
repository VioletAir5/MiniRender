#include "renderer/backends/opengl/passes/OpenGLGridPass.h"

#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/OpenGLShaderCache.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"

#include <glad/glad.h>

#include <string>

namespace renderlab {

OpenGLGridPass::OpenGLGridPass(OpenGLPassContext& context) noexcept : context_(context) {}

bool OpenGLGridPass::initialize() {
    return renderer_.initialize();
}

void OpenGLGridPass::shutdown() noexcept {
    renderer_.shutdown();
}

void OpenGLGridPass::execute(const RenderPassExecutionContext& execution) {
    const RenderFrame& frame = execution.frame;
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    std::string shaderError;
    OpenGLShaderProgram* shader =
        context_.shaderCache.resolve(context_.fallbackSurfaceShader, shaderError);
    if (shader == nullptr) {
        return;
    }
    shader->bind();
    shader->setInteger("uHasBaseColorTexture", 0);
    shader->setInteger("uHasMetallicRoughnessTexture", 0);
    shader->setInteger("uHasNormalTexture", 0);
    shader->setInteger("uAlphaMode", 0);
    shader->setInteger("uUnlit", 1);
    renderer_.render(*shader, frame.view, frame.projection);
}

} // namespace renderlab
