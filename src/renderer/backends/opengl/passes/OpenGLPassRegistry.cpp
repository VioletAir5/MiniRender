#include "renderer/backends/opengl/passes/OpenGLPassRegistry.h"

#include "renderer/backends/opengl/passes/OpenGLDirectionalShadowPass.h"
#include "renderer/backends/opengl/passes/OpenGLForwardPass.h"
#include "renderer/backends/opengl/passes/OpenGLGridPass.h"
#include "renderer/backends/opengl/passes/OpenGLOutlinePass.h"
#include "renderer/pipeline/BuiltInRenderPipeline.h"
#include "renderer/pipeline/RenderPipeline.h"

#include <memory>
#include <string>

namespace renderlab {

bool registerBuiltInOpenGLPasses(RenderPassFactory& factory, OpenGLPassContext& context) {
    const bool shadowRegistered =
        factory.registerType(std::string{render_pass_types::DirectionalShadow}, [&context] {
            return std::make_unique<OpenGLDirectionalShadowPass>(context);
        });
    const bool forwardRegistered =
        factory.registerType(std::string{render_pass_types::Forward},
                             [&context] { return std::make_unique<OpenGLForwardPass>(context); });
    const bool outlineRegistered =
        factory.registerType(std::string{render_pass_types::SelectionOutline},
                             [&context] { return std::make_unique<OpenGLOutlinePass>(context); });
    const bool gridRegistered =
        factory.registerType(std::string{render_pass_types::EditorGrid},
                             [&context] { return std::make_unique<OpenGLGridPass>(context); });
    return shadowRegistered && forwardRegistered && outlineRegistered && gridRegistered;
}

} // namespace renderlab
