#pragma once

#include <string_view>

namespace renderlab::asset_ids {

// 内置资产使用 URI 风格稳定标识，场景文件不保存运行期 Handle。
inline constexpr std::string_view UnitCube{"builtin:mesh/cube"};
inline constexpr std::string_view UnitPlane{"builtin:mesh/plane"};
inline constexpr std::string_view DefaultUvSphere{"builtin:mesh/uv-sphere/32x16"};
inline constexpr std::string_view DefaultMaterial{"builtin:material/default"};

} // namespace renderlab::asset_ids
