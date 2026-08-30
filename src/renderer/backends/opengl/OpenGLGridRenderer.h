#pragma once

#include <glad/glad.h>
#include <glm/mat4x4.hpp>

namespace renderlab {

class OpenGLShaderProgram;

// 绘制不属于场景资产的有限编辑器地面网格。
// 所有资源操作必须发生在所属 OpenGL 上下文有效时。
class OpenGLGridRenderer {
public:
    OpenGLGridRenderer() = default;
    ~OpenGLGridRenderer();

    OpenGLGridRenderer(const OpenGLGridRenderer&) = delete;
    OpenGLGridRenderer& operator=(const OpenGLGridRenderer&) = delete;

    // 创建 XZ 平面的线段顶点及对应 VAO/VBO。
    [[nodiscard]] bool initialize();

    // 释放网格拥有的 OpenGL 对象；可重复调用。
    void shutdown() noexcept;

    // 使用纯色 Shader 绘制小格、主线及 X/Z 坐标轴。
    void render(OpenGLShaderProgram& shader,
                const glm::mat4& view,
                const glm::mat4& projection) const;

    // 判断网格 GPU 资源是否已经就绪。
    [[nodiscard]] bool valid() const noexcept;

private:
    // 使用指定颜色绘制 VBO 中的一段连续线段。
    void drawRange(OpenGLShaderProgram& shader,
                   GLint first,
                   GLsizei count,
                   float red,
                   float green,
                   float blue,
                   float alpha) const;

    GLuint vertexArray_{0};
    GLuint vertexBuffer_{0};

    GLint minorFirst_{0};
    GLsizei minorCount_{0};
    GLint majorFirst_{0};
    GLsizei majorCount_{0};
    GLint xAxisFirst_{0};
    GLsizei xAxisCount_{0};
    GLint zAxisFirst_{0};
    GLsizei zAxisCount_{0};
};

} // namespace renderlab
