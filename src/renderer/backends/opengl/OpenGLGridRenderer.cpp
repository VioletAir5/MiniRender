#include "renderer/backends/opengl/OpenGLGridRenderer.h"

#include "renderer/backends/opengl/OpenGLShaderProgram.h"

#include <glm/vec3.hpp>

#include <vector>

namespace renderlab {
namespace {

// 第一版使用固定范围与刻度，后续可替换为随相机缩放的无限网格。
constexpr int kHalfExtent = 50;
constexpr int kMajorLineStep = 10;

void appendLine(std::vector<glm::vec3>& vertices,
                const glm::vec3& start,
                const glm::vec3& end) {
    vertices.push_back(start);
    vertices.push_back(end);
}

void appendLinesOfType(std::vector<glm::vec3>& vertices,
                       const bool majorLines) {
    for (int coordinate = -kHalfExtent;
         coordinate <= kHalfExtent;
         ++coordinate) {
        if (coordinate == 0 ||
            (coordinate % kMajorLineStep == 0) != majorLines) {
            continue;
        }

        const float value = static_cast<float>(coordinate);
        const float extent = static_cast<float>(kHalfExtent);

        appendLine(vertices,
                   {value, 0.0F, -extent},
                   {value, 0.0F, extent});
        appendLine(vertices,
                   {-extent, 0.0F, value},
                   {extent, 0.0F, value});
    }
}

} // namespace

OpenGLGridRenderer::~OpenGLGridRenderer() {
    shutdown();
}

bool OpenGLGridRenderer::initialize() {
    shutdown();

    std::vector<glm::vec3> vertices;
    vertices.reserve(404);

    minorFirst_ = static_cast<GLint>(vertices.size());
    appendLinesOfType(vertices, false);
    minorCount_ = static_cast<GLsizei>(vertices.size()) - minorFirst_;

    majorFirst_ = static_cast<GLint>(vertices.size());
    appendLinesOfType(vertices, true);
    majorCount_ = static_cast<GLsizei>(vertices.size()) - majorFirst_;

    const float extent = static_cast<float>(kHalfExtent);

    xAxisFirst_ = static_cast<GLint>(vertices.size());
    appendLine(vertices, {-extent, 0.0F, 0.0F}, {extent, 0.0F, 0.0F});
    xAxisCount_ = 2;

    zAxisFirst_ = static_cast<GLint>(vertices.size());
    appendLine(vertices, {0.0F, 0.0F, -extent}, {0.0F, 0.0F, extent});
    zAxisCount_ = 2;

    while (glGetError() != GL_NO_ERROR) {
    }

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    if (vertexArray_ == 0 || vertexBuffer_ == 0) {
        shutdown();
        return false;
    }

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
        vertices.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(glm::vec3)),
        nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        shutdown();
        return false;
    }

    return true;
}

void OpenGLGridRenderer::shutdown() noexcept {
    if (vertexBuffer_ != 0) {
        glDeleteBuffers(1, &vertexBuffer_);
        vertexBuffer_ = 0;
    }
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
    }

    minorFirst_ = 0;
    minorCount_ = 0;
    majorFirst_ = 0;
    majorCount_ = 0;
    xAxisFirst_ = 0;
    xAxisCount_ = 0;
    zAxisFirst_ = 0;
    zAxisCount_ = 0;
}

void OpenGLGridRenderer::render(
    OpenGLShaderProgram& shader,
    const glm::mat4& view,
    const glm::mat4& projection) const {
    if (!valid()) {
        return;
    }

    shader.bind();
    shader.setMatrix("uModel", glm::mat4{1.0F});
    shader.setMatrix("uView", view);
    shader.setMatrix("uProjection", projection);

    glBindVertexArray(vertexArray_);
    drawRange(shader, minorFirst_, minorCount_,
              0.42F, 0.44F, 0.48F, 0.28F);
    drawRange(shader, majorFirst_, majorCount_,
              0.58F, 0.60F, 0.64F, 0.48F);
    drawRange(shader, xAxisFirst_, xAxisCount_,
              0.90F, 0.22F, 0.20F, 0.90F);
    drawRange(shader, zAxisFirst_, zAxisCount_,
              0.20F, 0.42F, 0.92F, 0.90F);
    glBindVertexArray(0);

}

bool OpenGLGridRenderer::valid() const noexcept {
    return vertexArray_ != 0 && vertexBuffer_ != 0;
}

void OpenGLGridRenderer::drawRange(
    OpenGLShaderProgram& shader,
    const GLint first,
    const GLsizei count,
    const float red,
    const float green,
    const float blue,
    const float alpha) const {
    if (count <= 0) {
        return;
    }

    shader.setVector4("uBaseColor", {red, green, blue, alpha});
    glDrawArrays(GL_LINES, first, count);
}

} // namespace renderlab
