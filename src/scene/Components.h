#pragma once

#include "scene/EntityId.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace renderlab {
inline constexpr std::uint64_t MissingAsset = 0;
inline constexpr std::uint64_t BuiltinCubeMeshAsset = 1;


struct EntityMetadata {
    EntityId id{NullEntity};
    std::string name{"Entity"};
    EntityId parent{NullEntity};
    std::vector<EntityId> children;
};

struct TransformComponent {
    glm::vec3 position{0.0F, 0.0F, 0.0F};
    glm::vec3 rotationDegrees{0.0F, 0.0F, 0.0F};
    glm::vec3 scale{1.0F, 1.0F, 1.0F};
};

struct MeshRendererComponent {
    std::uint64_t meshAsset{MissingAsset};
    std::uint64_t materialAsset{MissingAsset};
    bool visible{true};
    bool castShadow{true};
};

struct CameraComponent {
    float verticalFovDegrees{60.0F};
    float nearPlane{0.1F};
    float farPlane{1000.0F};
    bool primary{false};
};

enum class LightType {
    Directional,
    Point,
    Spot,
};

struct LightComponent {
    LightType type{LightType::Directional};
    glm::vec3 color{1.0F, 1.0F, 1.0F};
    float intensity{1.0F};
    float range{10.0F};
};

} // namespace renderlab

