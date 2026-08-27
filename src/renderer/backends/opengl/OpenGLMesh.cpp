#include "renderer/backends/opengl/OpenGLMesh.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace renderlab {
namespace {

struct Vertex {
    float position[3];
    float color[3];
};

constexpr std::array<Vertex, 8> CubeVertices{{
    {{-0.5F, -0.5F, -0.5F}, {0.30F, 0.55F, 0.95F}},
    {{ 0.5F, -0.5F, -0.5F}, {0.35F, 0.85F, 0.75F}},
    {{ 0.5F,  0.5F, -0.5F}, {0.95F, 0.70F, 0.30F}},
    {{-0.5F,  0.5F, -0.5F}, {0.85F, 0.40F, 0.75F}},
    {{-0.5F, -0.5F,  0.5F}, {0.25F, 0.75F, 0.95F}},
    {{ 0.5F, -0.5F,  0.5F}, {0.45F, 0.90F, 0.45F}},
    {{ 0.5F,  0.5F,  0.5F}, {0.95F, 0.45F, 0.25F}},
    {{-0.5F,  0.5F,  0.5F}, {0.65F, 0.40F, 0.95F}},
}};

constexpr std::array<std::uint32_t, 36> CubeIndices{{
    0, 2, 1, 0, 3, 2,
    4, 5, 6, 4, 6, 7,
    0, 1, 5, 0, 5, 4,
    3, 7, 6, 3, 6, 2,
    0, 4, 7, 0, 7, 3,
    1, 2, 6, 1, 6, 5,
}};

void clearOpenGLErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}

} // namespace

bool OpenGLMesh::createCube() {
    destroy();
    clearOpenGLErrors();

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);

    glBindVertexArray(vertexArray_);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(CubeVertices.size() * sizeof(Vertex)),
                 CubeVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(CubeIndices.size() * sizeof(std::uint32_t)),
                 CubeIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, color)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    indexCount_ = static_cast<GLsizei>(CubeIndices.size());
    if (vertexArray_ == 0 || vertexBuffer_ == 0 || indexBuffer_ == 0 ||
        glGetError() != GL_NO_ERROR) {
        destroy();
        return false;
    }
    return true;
}

void OpenGLMesh::destroy() {
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

void OpenGLMesh::draw() const {
    if (vertexArray_ == 0 || indexCount_ == 0) {
        return;
    }

    glBindVertexArray(vertexArray_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace renderlab
