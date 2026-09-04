#pragma once

#include "renderer/pipeline/RenderPipeline.h"

#include <string_view>

namespace renderlab::render_pass_types {

// 稳定类型 ID 由每个图形后端注册同语义实现，使 Pipeline 描述不依赖具体 API。
inline constexpr std::string_view Forward = "renderlab.pass.forward";
inline constexpr std::string_view SelectionOutline = "renderlab.pass.selection-outline";
inline constexpr std::string_view EditorGrid = "renderlab.pass.editor-grid";

} // namespace renderlab::render_pass_types

namespace renderlab {

// 当前编辑器默认管线；未来可以由 RenderPipelineAsset 或用户配置替代。
[[nodiscard]] inline RenderPipelineDescriptor defaultEditorRenderPipeline() {
    return RenderPipelineDescriptor{
        .passes = {
            {.name = "Forward", .type = std::string{render_pass_types::Forward}},
            {.name = "Selection Outline", .type = std::string{render_pass_types::SelectionOutline}},
            {.name = "Editor Grid",
             .type = std::string{render_pass_types::EditorGrid},
             .required = false},
        }};
}

} // namespace renderlab
