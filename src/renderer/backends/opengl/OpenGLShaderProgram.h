#pragma once

#include <glad/glad.h>
#include <glm/mat4x4.hpp>

namespace renderlab {

class OpenGLShaderProgram {
public:
    OpenGLShaderProgram() = default;

    OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;

    bool initialize();
    void shutdown();

    void bind() const;
    static void release();
    void setMatrix(const char* name, const glm::mat4& value) const;

private:
    GLuint program_{0};
};

} // namespace renderlab
