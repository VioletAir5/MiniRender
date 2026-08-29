#include "renderer/backends/opengl/OpenGLMesh.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace renderlab {
namespace {

void clearOpenGLErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}

} // namespace

static_assert(std::is_standard_layout_v<Vertex>);

OpenGLMesh::~OpenGLMesh() {
    destroy();
}

OpenGLMesh::OpenGLMesh(OpenGLMesh&& other) noexcept
    : vertexArray_(std::exchange(other.vertexArray_, 0)),
      vertexBuffer_(std::exchange(other.vertexBuffer_, 0)),
      indexBuffer_(std::exchange(other.indexBuffer_, 0)),
      indexCount_(std::exchange(other.indexCount_, 0)) {
}

OpenGLMesh& OpenGLMesh::operator=(OpenGLMesh&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy();
    vertexArray_ = std::exchange(other.vertexArray_, 0);
    vertexBuffer_ = std::exchange(other.vertexBuffer_, 0);
    indexBuffer_ = std::exchange(other.indexBuffer_, 0);
    indexCount_ = std::exchange(other.indexCount_, 0);
    return *this;
}

bool OpenGLMesh::upload(const MeshPrimitive& primitive) {
    if (primitive.vertices.empty() || primitive.indices.empty()) {
        return false;
    }

    if (primitive.indices.size() >
        static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        return false;
    }

    const auto maxBufferSize =
        static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());
    if (primitive.vertices.size() > maxBufferSize / sizeof(Vertex) ||
        primitive.indices.size() > maxBufferSize / sizeof(std::uint32_t)) {
        return false;
    }

    for (const std::uint32_t index : primitive.indices) {
        if (index >= primitive.vertices.size()) {
            return false;
        }
    }

    destroy();
    clearOpenGLErrors();

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);

    if (vertexArray_ == 0 || vertexBuffer_ == 0 || indexBuffer_ == 0) {
        destroy();
        return false;
    }

    glBindVertexArray(vertexArray_);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(primitive.vertices.size() * sizeof(Vertex)),
        primitive.vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            primitive.indices.size() * sizeof(std::uint32_t)),
        primitive.indices.data(),
        GL_STATIC_DRAW);

    const auto stride = static_cast<GLsizei>(sizeof(Vertex));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(offsetof(Vertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(offsetof(Vertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 4, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(offsetof(Vertex, color)));

    indexCount_ = static_cast<GLsizei>(primitive.indices.size());

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        destroy();
        return false;
    }

    return true;
}

void OpenGLMesh::destroy() noexcept {
    if (indexBuffer_ != 0) {
        glDeleteBuffers(1, &indexBuffer_);
        indexBuffer_ = 0;
    }
    if (vertexBuffer_ != 0) {
        glDeleteBuffers(1, &vertexBuffer_);
        vertexBuffer_ = 0;
    }
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
    }
    indexCount_ = 0;
}

void OpenGLMesh::draw() const noexcept {
    if (!valid()) {
        return;
    }

    glBindVertexArray(vertexArray_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

bool OpenGLMesh::valid() const noexcept {
    return vertexArray_ != 0 && vertexBuffer_ != 0 &&
           indexBuffer_ != 0 && indexCount_ > 0;
}

} // namespace renderlab
