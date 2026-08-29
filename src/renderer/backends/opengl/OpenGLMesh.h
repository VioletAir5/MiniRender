#pragma once

#include "assets/MeshAsset.h"

#include <glad/glad.h>

namespace renderlab {

// 一个 MeshPrimitive 的 VAO/VBO/EBO 独占包装；不可复制但可移动。
// 析构和资源操作必须发生在所属 OpenGL 上下文有效时。
class OpenGLMesh {
public:
    OpenGLMesh() = default;
    ~OpenGLMesh();
    OpenGLMesh(const OpenGLMesh&) = delete;
    OpenGLMesh& operator=(const OpenGLMesh&) = delete;

    // 转移 OpenGL 对象名称，源对象会被置为空状态。
    OpenGLMesh(OpenGLMesh&& other) noexcept;
    OpenGLMesh& operator=(OpenGLMesh&& other) noexcept;

    // 校验 CPU 网格并上传到 GPU；失败后对象保持为空。
    bool upload(const MeshPrimitive& primitive);
    // 删除已拥有的 OpenGL 对象；可重复调用。
    void destroy() noexcept;
    // 绑定 VAO 并按三角形索引绘制；无效对象不执行操作。
    void draw() const noexcept;

    // 判断 VAO 和索引数量是否满足绘制条件。
    [[nodiscard]] bool valid() const noexcept;

private:
    GLuint vertexArray_{0};
    GLuint vertexBuffer_{0};
    GLuint indexBuffer_{0};
    GLsizei indexCount_{0};
};

} // namespace renderlab
