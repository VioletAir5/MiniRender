#pragma once

#include <glad/glad.h>

#include "renderer/surfaces/IRenderSurface.h"

#include <QOpenGLWidget>
#include <memory>

namespace renderlab {

class AssetRegistry;
class IRenderBackend;
class ShaderLibrary;

// 基于 QOpenGLWidget 的上下文表面；Qt 只管理窗口和上下文，不封装渲染命令。
class OpenGLRenderSurface final : public QOpenGLWidget, public IRenderSurface {
  public:
    // 创建使用共享资产注册表的 OpenGL 后端。
    explicit OpenGLRenderSurface(const AssetRegistry& registry, const ShaderLibrary& shaderLibrary,
                                 QWidget* parent = nullptr);
    ~OpenGLRenderSurface() override;

    // 实现 IRenderSurface 并返回自身作为嵌入控件。
    [[nodiscard]] QWidget& widget() noexcept override;
    // 替换待绘制帧；数据按值保存以隔离场景后续变化。
    void setFrame(RenderFrame frame) override;
    // 通过 QOpenGLWidget::update() 请求下一次 paintGL。
    void requestRender() override;

  protected:
    // 在 Qt 已激活 OpenGL 上下文后加载函数并初始化后端。
    void initializeGL() override;
    // 将物理视口尺寸转发给后端。
    void resizeGL(int width, int height) override;
    // 在当前上下文中提交最近保存的渲染帧。
    void paintGL() override;

  private:
    std::unique_ptr<IRenderBackend> backend_;
    RenderFrame frame_;
    bool backendReady_{false};
};

} // namespace renderlab
