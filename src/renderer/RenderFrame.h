#pragma once

#include "scene/EntityId.h"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <vector>

namespace renderlab {

struct RenderItem {
    EntityId entity{NullEntity};
    std::uint64_t meshAsset{0};
    std::uint64_t materialAsset{0};
    glm::mat4 model{1.0F};
};

struct RenderFrame {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
    std::vector<RenderItem> items;
    bool hasCamera{false};
};

} // namespace renderlab
