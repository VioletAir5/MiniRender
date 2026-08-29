#pragma once

#include "assets/AssetHandle.h"
#include "scene/EntityId.h"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <vector>

namespace renderlab {

// 场景提取后的单个绘制项，只保存后端需要的稳定句柄与模型矩阵。
struct RenderItem {
    EntityId entity{NullEntity};
    MeshHandle meshAsset;
    MaterialHandle materialAsset;
    glm::mat4 model{1.0F};
};

// 一帧不可变的 API 无关渲染快照，由 SceneRenderer 生成并交给渲染表面。
struct RenderFrame {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
    std::vector<RenderItem> items;
    bool hasCamera{false};
};

} // namespace renderlab
