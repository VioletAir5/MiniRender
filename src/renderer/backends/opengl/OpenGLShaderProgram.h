#pragma once

#include <QOpenGLShaderProgram>

#include <glm/mat4x4.hpp>

class QOpenGLFunctions;

namespace renderlab {

class OpenGLShaderProgram {
public:
    bool initialize();
    void shutdown();

    void bind();
    void release();
    void setMatrix(QOpenGLFunctions& functions, const char* name,
                   const glm::mat4& value);

private:
    QOpenGLShaderProgram program_;
};

} // namespace renderlab
