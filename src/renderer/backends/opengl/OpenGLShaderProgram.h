#pragma once

#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace renderlab {

// 基础网格着色器程序的独占包装；资源操作要求有效 OpenGL 上下文。
class OpenGLShaderProgram {
public:
    OpenGLShaderProgram() = default;

    OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;

    // 编译并链接内置顶点/片元着色器。
    bool initialize();
    // 删除程序对象并恢复为空状态。
    void shutdown();

    // 绑定该程序用于后续绘制。
    void bind() const;
    // 解绑当前着色器程序。
    static void release();
    // 按 uniform 名称上传 4x4 矩阵；名称不存在时忽略。
    void setMatrix(const char* name, const glm::mat4& value) const;
    // 按 uniform 名称上传纯色材质颜色；名称不存在时忽略。
    void setVector4(const char* name, const glm::vec4& value) const;

private:
    GLuint program_{0};
};

} // namespace renderlab
