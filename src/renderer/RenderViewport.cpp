#include "renderer/RenderViewport.h"

#include "renderer/surfaces/IRenderSurface.h"
#include "renderer/surfaces/OpenGLRenderSurface.h"

#include <QVBoxLayout>

namespace renderlab {

RenderViewport::RenderViewport(QWidget* parent)
    : QWidget(parent),
      surface_(std::make_unique<OpenGLRenderSurface>(this)) {
    setMinimumSize(640, 360);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(&surface_->widget());
}

RenderViewport::~RenderViewport() = default;

void RenderViewport::setScene(const SceneDocument* scene) noexcept {
    surface_->setScene(scene);
}

void RenderViewport::requestRender() {
    surface_->requestRender();
}

} // namespace renderlab

