#include "renderer/backends/opengl/OpenGLShaderProgram.h"

#include <QDebug>
#include <QOpenGLFunctions>
#include <QOpenGLShader>

#include <glm/gtc/type_ptr.hpp>

namespace renderlab {
namespace {

constexpr auto VertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vertexColor;

void main()
{
    vertexColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";

constexpr auto FragmentShaderSource = R"(
#version 330 core

in vec3 vertexColor;
out vec4 fragColor;

void main()
{
    fragColor = vec4(vertexColor, 1.0);
}
)";

} // namespace

bool OpenGLShaderProgram::initialize() {
    const bool vertexCompiled =
        program_.addShaderFromSourceCode(QOpenGLShader::Vertex, VertexShaderSource);
    const bool fragmentCompiled =
        program_.addShaderFromSourceCode(QOpenGLShader::Fragment, FragmentShaderSource);
    const bool linked = vertexCompiled && fragmentCompiled && program_.link();

    if (!linked) {
        qWarning().noquote() << "OpenGL shader initialization failed:" << program_.log();
    }
    return linked;
}

void OpenGLShaderProgram::shutdown() {
    program_.removeAllShaders();
}

void OpenGLShaderProgram::bind() {
    program_.bind();
}

void OpenGLShaderProgram::release() {
    program_.release();
}

void OpenGLShaderProgram::setMatrix(QOpenGLFunctions& functions, const char* name,
                                    const glm::mat4& value) {
    const int location = program_.uniformLocation(name);
    if (location >= 0) {
        functions.glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

} // namespace renderlab
