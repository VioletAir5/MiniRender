#pragma once

#include "assets/MeshAsset.h"

#include <glad/glad.h>

namespace renderlab {

class OpenGLMesh {
public:
    OpenGLMesh() = default;
    ~OpenGLMesh();
    OpenGLMesh(const OpenGLMesh&) = delete;
    OpenGLMesh& operator=(const OpenGLMesh&) = delete;

    OpenGLMesh(OpenGLMesh&& other) noexcept;
    OpenGLMesh& operator=(OpenGLMesh&& other) noexcept;

    bool upload(const MeshPrimitive& primitive);
    void destroy() noexcept;
    void draw() const noexcept;

    [[nodiscard]] bool valid() const noexcept;

private:
    GLuint vertexArray_{0};
    GLuint vertexBuffer_{0};
    GLuint indexBuffer_{0};
    GLsizei indexCount_{0};
};

} // namespace renderlab
