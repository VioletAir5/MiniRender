#include "renderer/backends/opengl/OpenGLShaderProgram.h"

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

namespace renderlab {
namespace {

constexpr std::string_view VertexShaderSource = R"(
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

constexpr std::string_view FragmentShaderSource = R"(
#version 330 core

in vec3 vertexColor;
out vec4 fragColor;

void main()
{
    fragColor = vec4(vertexColor, 1.0);
}
)";

const char* shaderStageName(const GLenum stage) {
    return stage == GL_VERTEX_SHADER ? "vertex" : "fragment";
}

GLuint compileShader(const GLenum stage, const std::string_view source) {
    const GLuint shader = glCreateShader(stage);
    const GLchar* sourceData = source.data();
    const GLint sourceLength = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &sourceData, &sourceLength);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    spdlog::error("OpenGL {} shader compilation failed: {}", shaderStageName(stage), log);
    glDeleteShader(shader);
    return 0;
}

void logProgramLinkError(const GLuint program) {
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    glGetProgramInfoLog(program, logLength, nullptr, log.data());
    spdlog::error("OpenGL program link failed: {}", log);
}

} // namespace

bool OpenGLShaderProgram::initialize() {
    shutdown();

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VertexShaderSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FragmentShaderSource);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
        }
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        logProgramLinkError(program_);
        shutdown();
        return false;
    }
    return true;
}

void OpenGLShaderProgram::shutdown() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void OpenGLShaderProgram::bind() const {
    glUseProgram(program_);
}

void OpenGLShaderProgram::release() {
    glUseProgram(0);
}

void OpenGLShaderProgram::setMatrix(const char* name, const glm::mat4& value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

} // namespace renderlab
