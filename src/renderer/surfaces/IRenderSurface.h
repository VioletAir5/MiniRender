#pragma once

#include "renderer/RenderFrame.h"

class QWidget;

namespace renderlab {

// 连接 Qt 窗口与图形上下文的抽象表面，负责驱动具体后端。
class IRenderSurface {
public:
    virtual ~IRenderSurface() = default;

    // 返回可嵌入编辑器布局的原生 Qt 控件。
    [[nodiscard]] virtual QWidget& widget() noexcept = 0;
    // 保存下一次重绘要消费的帧快照。
    virtual void setFrame(RenderFrame frame) = 0;
    // 请求异步重绘，不保证立即执行。
    virtual void requestRender() = 0;
};

} // namespace renderlab
