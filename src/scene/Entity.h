#pragma once

#include "scene/EntityId.h"

namespace renderlab {

// 场景实体的轻量值对象；实际元数据和组件均由 SceneDocument 持有。
struct Entity {
    EntityId id{NullEntity};

    // 仅当对象引用了一个非空实体标识时返回 true。
    [[nodiscard]] explicit operator bool() const noexcept {
        return id != NullEntity;
    }

    // 实体仅按稳定标识比较。
    auto operator<=>(const Entity&) const = default;
};

} // namespace renderlab

