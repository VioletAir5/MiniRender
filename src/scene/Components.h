#pragma once

#include "assets/AssetHandle.h"
#include "scene/EntityId.h"
#include "scene/LightTypes.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace renderlab {

// 保留给内置立方体的历史句柄；新代码应优先从 ProceduralMeshLibrary 获取句柄。
inline constexpr MeshHandle BuiltinCubeMeshAsset{1, 1};

// 保存实体名称及父子层级关系。
struct EntityMetadata {
    EntityId id{NullEntity};
    std::string name{"Entity"};
    EntityId parent{NullEntity};
    std::vector<EntityId> children;
};

// 描述实体相对于父节点的平移、欧拉角旋转和缩放。
struct TransformComponent {
    glm::vec3 position{0.0F, 0.0F, 0.0F};
    glm::vec3 rotationDegrees{0.0F, 0.0F, 0.0F};
    glm::vec3 scale{1.0F, 1.0F, 1.0F};
};

// 将一个网格及可选材质绑定到实体，供渲染帧提取使用。
struct MeshRendererComponent {
    MeshHandle meshAsset;
    MaterialHandle materialAsset;
    bool visible{true};
    bool castShadow{true};
};

// 描述透视相机的投影参数；primary 标记场景默认相机。
struct CameraComponent {
    float verticalFovDegrees{60.0F};
    float nearPlane{0.1F};
    float farPlane{1000.0F};
    bool primary{false};
};

// 保存光源类型、颜色及衰减相关参数。
struct LightComponent {
    LightType type{LightType::Directional};
    glm::vec3 color{1.0F, 1.0F, 1.0F};
    float intensity{1.0F};
    float range{10.0F};
    float innerConeDegrees{20.0F};
    float outerConeDegrees{30.0F};
    bool castShadow{true};
    ShadowTechnique shadowTechnique{ShadowTechnique::Pcf};
    float shadowBias{0.0015F};
    float shadowDistance{50.0F};
};

} // namespace renderlab
