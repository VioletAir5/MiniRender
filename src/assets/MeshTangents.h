#pragma once

#include "assets/MeshAsset.h"

namespace renderlab {

// 根据三角形位置、法线和 UV 生成每顶点切线及副切线手性。
// 返回 false 表示没有可用于计算的非退化 UV 三角形，此时仍会写入稳定的备用切线。
bool generateTangents(MeshPrimitive& primitive) noexcept;

} // namespace renderlab
