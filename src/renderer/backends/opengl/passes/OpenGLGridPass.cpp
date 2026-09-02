#include "renderer/backends/opengl/passes/OpenGLGridPass.h"

#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"

#include <glad/glad.h>

namespace renderlab {

bool OpenGLGridPass::initialize() {
    return renderer_.initialize();
}

void OpenGLGridPass::shutdown() noexcept {
    renderer_.shutdown();
}

void OpenGLGridPass::render(
    const RenderFrame& frame, OpenGLPassContext& context) {
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    OpenGLShaderProgram& shader = context.shader;
    shader.setInteger("uHasBaseColorTexture", 0);
    shader.setInteger("uHasMetallicRoughnessTexture", 0);
    shader.setInteger("uHasNormalTexture", 0);
    shader.setInteger("uAlphaMode", 0);
    shader.setInteger("uUnlit", 1);
    renderer_.render(shader, frame.view, frame.projection);
}

} // namespace renderlab
