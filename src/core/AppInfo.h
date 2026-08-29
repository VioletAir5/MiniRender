#pragma once

#include <string_view>

namespace renderlab {

// 返回用于窗口标题和日志标识的应用名称。
[[nodiscard]] constexpr std::string_view applicationName() noexcept {
    return "RenderLab";
}

// 返回遵循语义化版本格式的应用版本号。
[[nodiscard]] std::string_view applicationVersion() noexcept;

} // namespace renderlab

