#include "renderer/backends/opengl/passes/OpenGLForwardPass.h"

#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"
#include "renderer/backends/opengl/passes/OpenGLSceneDraw.h"

#include <glad/glad.h>

namespace renderlab {

const RenderItem* OpenGLForwardPass::render(
    const RenderFrame& frame, OpenGLPassContext& context) const {
    OpenGLShaderProgram& shader = context.shader;
    shader.setVector3("uCameraPosition", frame.cameraPosition);
    shader.setInteger("uHasDirectionalLight",
                      frame.directionalLight.valid ? 1 : 0);
    shader.setVector3("uLightDirection", frame.directionalLight.direction);
    shader.setVector3("uLightColor", frame.directionalLight.color);
    shader.setFloat("uLightIntensity", frame.directionalLight.intensity);
    shader.setMatrix("uView", frame.view);
    shader.setMatrix("uProjection", frame.projection);
    shader.setInteger("uBaseColorTexture", 0);
    shader.setInteger("uMetallicRoughnessTexture", 1);
    shader.setInteger("uNormalTexture", 2);

    const SelectionOutline* outline = frame.selectionOutline.has_value()
                                          ? &*frame.selectionOutline
                                          : nullptr;
    const RenderItem* outlinedItem = nullptr;
    if (outline != nullptr) {
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    for (const RenderItem& item : frame.items) {
        const bool selected = outline != nullptr &&
                              item.entity == outline->entity;
        if (outline != nullptr) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(selected ? 0xFF : 0x00);
        }
        drawOpenGLSceneItem(context, item, item.model);
        if (selected) {
            outlinedItem = &item;
        }
    }
    return outlinedItem;
}

} // namespace renderlab
