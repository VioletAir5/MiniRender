#include "renderer/backends/opengl/OpenGLMesh.h"

#include <QOpenGLFunctions>

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

} // namespace

OpenGLMesh::OpenGLMesh()
    : vertexBuffer_(QOpenGLBuffer::VertexBuffer),
      indexBuffer_(QOpenGLBuffer::IndexBuffer) {}

bool OpenGLMesh::createCube(QOpenGLFunctions& functions) {
    if (!vertexArray_.create() || !vertexBuffer_.create() || !indexBuffer_.create()) {
        destroy();
        return false;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vertexArray_);

    vertexBuffer_.bind();
    vertexBuffer_.allocate(CubeVertices.data(),
                           static_cast<int>(CubeVertices.size() * sizeof(Vertex)));

    indexBuffer_.bind();
    indexBuffer_.allocate(CubeIndices.data(),
                          static_cast<int>(CubeIndices.size() * sizeof(std::uint32_t)));

    functions.glEnableVertexAttribArray(0);
    functions.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                    reinterpret_cast<const void*>(offsetof(Vertex, position)));
    functions.glEnableVertexAttribArray(1);
    functions.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                    reinterpret_cast<const void*>(offsetof(Vertex, color)));

    vertexBuffer_.release();
    indexCount_ = static_cast<int>(CubeIndices.size());
    return true;
}

void OpenGLMesh::destroy() {
    indexCount_ = 0;
    if (indexBuffer_.isCreated()) {
        indexBuffer_.destroy();
    }
    if (vertexBuffer_.isCreated()) {
        vertexBuffer_.destroy();
    }
    if (vertexArray_.isCreated()) {
        vertexArray_.destroy();
    }
}

void OpenGLMesh::draw(QOpenGLFunctions& functions) {
    if (indexCount_ == 0) {
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vertexArray_);
    functions.glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}

} // namespace renderlab
