#pragma once

#include "assets/AssetHandle.h"
#include "scene/EntityId.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace renderlab {

// 场景提取后的单个绘制项，只保存后端需要的稳定句柄与模型矩阵。
struct RenderItem {
    EntityId entity{NullEntity};
    MeshHandle meshAsset;
    MaterialHandle materialAsset;
    glm::mat4 model{1.0F};
};

// 编辑器请求后端为指定实体绘制轮廓，不包含任何具体图形 API 状态。
struct SelectionOutline {
    // 通过稳定实体 ID 关联本帧的 RenderItem。
    EntityId entity{NullEntity};
    // 线性空间 RGBA 轮廓颜色，默认使用醒目的编辑器橙色。
    glm::vec4 color{1.0F, 0.55F, 0.10F, 1.0F};
    // 绕网格局部原点的膨胀比例；1.04 表示放大百分之四。
    float scale{1.04F};
};

// 一帧不可变的 API 无关渲染快照，由 SceneRenderer 生成并交给渲染表面。
struct RenderFrame {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
    std::vector<RenderItem> items;
    std::optional<SelectionOutline> selectionOutline;
    bool hasCamera{false};
};

} // namespace renderlab
