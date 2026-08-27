#pragma once

#include <glad/glad.h>

namespace renderlab {

class OpenGLMesh {
public:
    OpenGLMesh() = default;

    OpenGLMesh(const OpenGLMesh&) = delete;
    OpenGLMesh& operator=(const OpenGLMesh&) = delete;

    bool createCube();
    void destroy();
    void draw() const;

private:
    GLuint vertexArray_{0};
    GLuint vertexBuffer_{0};
    GLuint indexBuffer_{0};
    GLsizei indexCount_{0};
};

} // namespace renderlab
