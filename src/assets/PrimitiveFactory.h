#pragma once

#include "assets/MeshAsset.h"

#include <cstdint>

namespace renderlab::primitive_factory {

// 在 XZ 平面生成中心位于原点、法线朝 +Y 的矩形网格。
[[nodiscard]] MeshPrimitive createPlane(float width = 1.0F,
                                        float depth = 1.0F);

// 生成中心位于原点的硬边立方体，每个面拥有独立法线和 UV。
[[nodiscard]] MeshPrimitive createCube(float size = 1.0F);

// 生成以 Y 轴为极轴的 UV 球；细分数会被限制到有效范围。
[[nodiscard]] MeshPrimitive createUvSphere(float radius = 0.5F,
                                           std::uint32_t segments = 32,
                                           std::uint32_t rings = 16);

} // namespace renderlab::primitive_factory
