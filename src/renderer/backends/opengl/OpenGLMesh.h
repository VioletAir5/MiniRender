#pragma once

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class QOpenGLFunctions;

namespace renderlab {

class OpenGLMesh {
public:
    OpenGLMesh();

    bool createCube(QOpenGLFunctions& functions);
    void destroy();
    void draw(QOpenGLFunctions& functions);

private:
    QOpenGLVertexArrayObject vertexArray_;
    QOpenGLBuffer vertexBuffer_;
    QOpenGLBuffer indexBuffer_;
    int indexCount_{0};
};

} // namespace renderlab
