#pragma once

#include "scene/EntityId.h"

namespace renderlab {

// A lightweight reference to scene data. SceneDocument owns the actual entity
// metadata and components.
struct Entity {
    EntityId id{NullEntity};

    [[nodiscard]] explicit operator bool() const noexcept {
        return id != NullEntity;
    }

    auto operator<=>(const Entity&) const = default;
};

} // namespace renderlab

