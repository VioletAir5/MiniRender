#pragma once

#include <string_view>

namespace renderlab {

struct RenderPassContext;

// API 无关的渲染阶段；只表达渲染策略，不直接调用原生图形 API。
class IRenderPass {
public:
    virtual ~IRenderPass() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    virtual void execute(RenderPassContext& context) = 0;

    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    void setEnabled(const bool enabled) noexcept { enabled_ = enabled; }

private:
    bool enabled_{true};
};

} // namespace renderlab
