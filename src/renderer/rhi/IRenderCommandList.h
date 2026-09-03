#pragma once

#include "renderer/rhi/RenderStates.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <optional>

namespace renderlab {

struct RenderItem;

struct MeshDrawParameters {
    glm::mat4 model{1.0F};
    std::optional<glm::vec4> overrideColor;
};

// API 无关的逐帧绘制命令接口。具体后端负责把状态和绘制请求翻译为原生 API。
class IRenderCommandList {
public:
    virtual ~IRenderCommandList() = default;

    virtual void setStencilState(const StencilState& state) = 0;
    virtual void setDepthWriteEnabled(bool enabled) = 0;
    virtual void setBlendEnabled(bool enabled) = 0;
    virtual void setCullEnabled(bool enabled) = 0;

    virtual void drawMesh(const RenderItem& item,
                          const MeshDrawParameters& parameters) = 0;
    virtual void drawEditorGrid(const glm::mat4& view,
                                const glm::mat4& projection) = 0;
};

} // namespace renderlab
