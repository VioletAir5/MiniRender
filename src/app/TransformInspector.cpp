#include "app/TransformInspector.h"

#include "editor/TransformCommand.h"
#include "scene/SceneDocument.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QStringList>
#include <QUndoStack>
#include <QVBoxLayout>

#include <glm/vec3.hpp>

#include <cstddef>

namespace renderlab {
namespace {

// 汇总实体已拥有的组件名称，保留原 Inspector 的基础信息。
QString componentSummary(const SceneDocument& scene, const EntityId entity) {
    QStringList components{QStringLiteral("Transform")};

    if (scene.tryGetMeshRenderer(entity) != nullptr) {
        components.push_back(QStringLiteral("Mesh Renderer"));
    }
    if (scene.tryGetCamera(entity) != nullptr) {
        components.push_back(QStringLiteral("Camera"));
    }
    if (scene.tryGetLight(entity) != nullptr) {
        components.push_back(QStringLiteral("Light"));
    }

    return components.join(QStringLiteral(", "));
}

struct VectorEditorOptions {
    double sliderMinimum;
    double sliderMaximum;
    double sliderStep;
    QString suffix;
};

// 每个轴独占一行，用颜色区分轴向，并同时提供滑条和精确数值输入。
QGroupBox* createVectorEditor(const QString& title, QWidget* parent,
                              std::array<QDoubleSpinBox*, 3>& fields,
                              std::array<QSlider*, 3>& sliders,
                              const QString& objectPrefix,
                              const VectorEditorOptions& options) {
    auto* editor = new QGroupBox(title, parent);
    editor->setObjectName(objectPrefix + QStringLiteral("Group"));
    auto* layout = new QGridLayout(editor);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(5);
    layout->setColumnStretch(1, 1);

    constexpr std::array<const char*, 3> AxisNames{"X", "Y", "Z"};
    constexpr std::array<const char*, 3> AxisColors{"#e06c75", "#98c379", "#61afef"};
    const int sliderMinimum = static_cast<int>(options.sliderMinimum / options.sliderStep);
    const int sliderMaximum = static_cast<int>(options.sliderMaximum / options.sliderStep);

    for (std::size_t index = 0; index < fields.size(); ++index) {
        auto* axis = new QLabel(QString::fromLatin1(AxisNames[index]), editor);
        axis->setAlignment(Qt::AlignCenter);
        axis->setFixedSize(22, 22);
        axis->setStyleSheet(QStringLiteral("font-weight: 700; color: white; border-radius: 3px; "
                                           "background-color: %1;")
                                .arg(QString::fromLatin1(AxisColors[index])));
        layout->addWidget(axis, static_cast<int>(index), 0);

        sliders[index] = new QSlider(Qt::Horizontal, editor);
        sliders[index]->setObjectName(objectPrefix +
                                      QString::fromLatin1(AxisNames[index]) + QStringLiteral("Slider"));
        sliders[index]->setRange(sliderMinimum, sliderMaximum);
        sliders[index]->setSingleStep(1);
        sliders[index]->setPageStep(10);
        sliders[index]->setTracking(true);
        layout->addWidget(sliders[index], static_cast<int>(index), 1);

        fields[index] = new QDoubleSpinBox(editor);
        fields[index]->setObjectName(objectPrefix +
                                     QString::fromLatin1(AxisNames[index]) + QStringLiteral("Field"));
        fields[index]->setDecimals(3);
        fields[index]->setRange(-1000000.0, 1000000.0);
        fields[index]->setSingleStep(options.sliderStep);
        fields[index]->setAccelerated(true);
        fields[index]->setKeyboardTracking(false);
        fields[index]->setMinimumWidth(92);
        fields[index]->setSuffix(options.suffix);
        layout->addWidget(fields[index], static_cast<int>(index), 2);

        QSlider* slider = sliders[index];
        QDoubleSpinBox* field = fields[index];
        QObject::connect(slider, &QSlider::valueChanged, field,
                         [field, step = options.sliderStep](const int value) {
                             field->setValue(static_cast<double>(value) * step);
                         });
        QObject::connect(field, &QDoubleSpinBox::valueChanged, slider,
                         [slider, step = options.sliderStep](const double value) {
                             const QSignalBlocker blocker{slider};
                             slider->setValue(qRound(value / step));
                         });
    }

    return editor;
}

glm::vec3 fieldValues(const std::array<QDoubleSpinBox*, 3>& fields) {
    return {
        static_cast<float>(fields[0]->value()),
        static_cast<float>(fields[1]->value()),
        static_cast<float>(fields[2]->value()),
    };
}

void setFieldValues(const std::array<QDoubleSpinBox*, 3>& fields,
                    const std::array<QSlider*, 3>& sliders, const glm::vec3& values,
                    const double sliderStep) {
    const std::array<double, 3> components{values.x, values.y, values.z};
    for (std::size_t index = 0; index < fields.size(); ++index) {
        const QSignalBlocker fieldBlocker{fields[index]};
        const QSignalBlocker sliderBlocker{sliders[index]};
        fields[index]->setValue(components[index]);
        sliders[index]->setValue(qRound(components[index] / sliderStep));
    }
}

bool transformEquals(const TransformComponent& left, const TransformComponent& right) {
    return left.position.x == right.position.x && left.position.y == right.position.y &&
           left.position.z == right.position.z &&
           left.rotationDegrees.x == right.rotationDegrees.x &&
           left.rotationDegrees.y == right.rotationDegrees.y &&
           left.rotationDegrees.z == right.rotationDegrees.z && left.scale.x == right.scale.x &&
           left.scale.y == right.scale.y && left.scale.z == right.scale.z;
}

bool editorIsActive(const std::array<QDoubleSpinBox*, 3>& fields,
                    const std::array<QSlider*, 3>& sliders) {
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (fields[index]->hasFocus() || sliders[index]->isSliderDown()) {
            return true;
        }
    }
    return false;
}

} // namespace

TransformInspector::TransformInspector(QWidget* parent) : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    summaryLabel_ = new QLabel(tr("No object selected"), this);
    summaryLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    summaryLabel_->setWordWrap(true);
    rootLayout->addWidget(summaryLabel_);

    transformGroup_ = new QGroupBox(tr("Transform"), this);
    auto* transformLayout = new QVBoxLayout(transformGroup_);
    transformLayout->setContentsMargins(6, 8, 6, 6);
    transformLayout->setSpacing(7);
    transformLayout->addWidget(createVectorEditor(tr("Position"), transformGroup_, positionFields_,
                                                  positionSliders_, QStringLiteral("position"),
                                                  {-100.0, 100.0, 0.01, {}}));
    transformLayout->addWidget(createVectorEditor(tr("Rotation"), transformGroup_, rotationFields_,
                                                  rotationSliders_, QStringLiteral("rotation"),
                                                  {-180.0, 180.0, 0.1, QStringLiteral("°")}));
    transformLayout->addWidget(createVectorEditor(tr("Scale"), transformGroup_, scaleFields_,
                                                  scaleSliders_, QStringLiteral("scale"),
                                                  {-10.0, 10.0, 0.01, {}}));
    rootLayout->addWidget(transformGroup_);
    rootLayout->addStretch(1);

    for (QDoubleSpinBox* field : positionFields_) {
        connect(field, &QDoubleSpinBox::valueChanged, this, [this](double) { applyPosition(); });
        connect(field, &QDoubleSpinBox::editingFinished, this,
                &TransformInspector::endTransformEdit);
    }
    for (QDoubleSpinBox* field : rotationFields_) {
        connect(field, &QDoubleSpinBox::valueChanged, this, [this](double) { applyRotation(); });
        connect(field, &QDoubleSpinBox::editingFinished, this,
                &TransformInspector::endTransformEdit);
    }
    for (QDoubleSpinBox* field : scaleFields_) {
        connect(field, &QDoubleSpinBox::valueChanged, this, [this](double) { applyScale(); });
        connect(field, &QDoubleSpinBox::editingFinished, this,
                &TransformInspector::endTransformEdit);
    }

    for (QSlider* slider : positionSliders_) {
        connect(slider, &QSlider::sliderPressed, this,
                [this] { beginTransformEdit(tr("Change Position")); });
        connect(slider, &QSlider::sliderReleased, this, &TransformInspector::endTransformEdit);
    }
    for (QSlider* slider : rotationSliders_) {
        connect(slider, &QSlider::sliderPressed, this,
                [this] { beginTransformEdit(tr("Change Rotation")); });
        connect(slider, &QSlider::sliderReleased, this, &TransformInspector::endTransformEdit);
    }
    for (QSlider* slider : scaleSliders_) {
        connect(slider, &QSlider::sliderPressed, this,
                [this] { beginTransformEdit(tr("Change Scale")); });
        connect(slider, &QSlider::sliderReleased, this, &TransformInspector::endTransformEdit);
    }

    transformGroup_->setEnabled(false);
}

void TransformInspector::setUndoStack(QUndoStack* undoStack) {
    commitPendingEdit();
    undoStack_ = undoStack;
}

void TransformInspector::setEntity(SceneDocument* scene, const EntityId entity) {
    commitPendingEdit();
    scene_ = scene;
    entity_ = scene_ != nullptr && scene_->contains(entity) ? entity : NullEntity;
    refresh();
}

void TransformInspector::commitPendingEdit() {
    QWidget* focused = QApplication::focusWidget();
    if (focused != nullptr && (focused == this || isAncestorOf(focused))) {
        focused->clearFocus();
    }
    endTransformEdit();
}

void TransformInspector::refresh() {
    if (scene_ == nullptr || entity_ == NullEntity) {
        summaryLabel_->setText(tr("No object selected"));
        transformGroup_->setEnabled(false);
        return;
    }

    const EntityMetadata* metadata = scene_->tryGetEntity(entity_);
    const TransformComponent* transform = scene_->tryGetTransform(entity_);
    if (metadata == nullptr || transform == nullptr) {
        summaryLabel_->setText(tr("No object selected"));
        transformGroup_->setEnabled(false);
        return;
    }

    summaryLabel_->setText(tr("Name: %1\nEntity ID: %2\nComponents: %3")
                               .arg(QString::fromStdString(metadata->name))
                               .arg(entity_)
                               .arg(componentSummary(*scene_, entity_)));

    setFieldValues(positionFields_, positionSliders_, transform->position, 0.01);
    setFieldValues(rotationFields_, rotationSliders_, transform->rotationDegrees, 0.1);
    setFieldValues(scaleFields_, scaleSliders_, transform->scale, 0.01);
    transformGroup_->setEnabled(true);
}

bool TransformInspector::beginTransformEdit(const QString& text) {
    if (editActive_) {
        return false;
    }

    const TransformComponent* transform =
        scene_ == nullptr ? nullptr : scene_->tryGetTransform(entity_);
    if (transform == nullptr) {
        return false;
    }

    editScene_ = scene_;
    editEntity_ = entity_;
    editBefore_ = *transform;
    editText_ = text;
    editActive_ = true;
    return true;
}

void TransformInspector::endTransformEdit() {
    if (!editActive_) {
        return;
    }

    SceneDocument* editScene = editScene_;
    const EntityId editEntity = editEntity_;
    const TransformComponent before = editBefore_;
    const QString text = editText_;
    const TransformComponent* current =
        editScene == nullptr ? nullptr : editScene->tryGetTransform(editEntity);

    editActive_ = false;
    editScene_ = nullptr;
    editEntity_ = NullEntity;
    editText_.clear();

    if (undoStack_ == nullptr || current == nullptr || transformEquals(before, *current)) {
        return;
    }

    undoStack_->push(new TransformCommand(*editScene, editEntity, before, *current, text));
}

void TransformInspector::applyPosition() {
    const bool started = beginTransformEdit(tr("Change Position"));
    if (scene_ != nullptr && entity_ != NullEntity &&
        scene_->setPosition(entity_, fieldValues(positionFields_))) {
        emit transformEdited(entity_);
    }
    if (started && !editorIsActive(positionFields_, positionSliders_)) {
        endTransformEdit();
    }
}

void TransformInspector::applyRotation() {
    const bool started = beginTransformEdit(tr("Change Rotation"));
    if (scene_ != nullptr && entity_ != NullEntity &&
        scene_->setRotation(entity_, fieldValues(rotationFields_))) {
        emit transformEdited(entity_);
    }
    if (started && !editorIsActive(rotationFields_, rotationSliders_)) {
        endTransformEdit();
    }
}

void TransformInspector::applyScale() {
    const bool started = beginTransformEdit(tr("Change Scale"));
    if (scene_ != nullptr && entity_ != NullEntity &&
        scene_->setScale(entity_, fieldValues(scaleFields_))) {
        emit transformEdited(entity_);
    }
    if (started && !editorIsActive(scaleFields_, scaleSliders_)) {
        endTransformEdit();
    }
}

} // namespace renderlab
