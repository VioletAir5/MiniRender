#pragma once

#include <string_view>

namespace renderlab {

[[nodiscard]] constexpr std::string_view applicationName() noexcept {
    return "RenderLab";
}

[[nodiscard]] std::string_view applicationVersion() noexcept;

} // namespace renderlab

