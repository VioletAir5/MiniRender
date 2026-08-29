#pragma once

#include <cstdint>

namespace renderlab {

// 场景实体的稳定标识；在同一个 SceneDocument 的生命周期内不会重复使用。
using EntityId = std::uint64_t;

// 表示空实体，也用于标记根实体没有父节点。
inline constexpr EntityId NullEntity = 0;

} // namespace renderlab

