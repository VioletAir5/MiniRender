#pragma once

#include "assets/MeshAsset.h"

#include <cstdint>

namespace renderlab::primitive_factory {

[[nodiscard]] MeshPrimitive createPlane(float width = 1.0F,
                                        float depth = 1.0F);

[[nodiscard]] MeshPrimitive createCube(float size = 1.0F);

[[nodiscard]] MeshPrimitive createUvSphere(float radius = 0.5F,
                                           std::uint32_t segments = 32,
                                           std::uint32_t rings = 16);

} // namespace renderlab::primitive_factory
