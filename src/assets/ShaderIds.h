#pragma once

#include <string_view>

namespace renderlab::shader_asset_ids {

inline constexpr std::string_view PbrForward = "renderlab.shader.pbr-forward";
inline constexpr std::string_view DirectionalShadowDepth =
    "renderlab.shader.directional-shadow-depth";

} // namespace renderlab::shader_asset_ids
