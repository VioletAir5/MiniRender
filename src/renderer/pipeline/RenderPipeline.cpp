#include "renderer/pipeline/RenderPipeline.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace renderlab {

bool RenderPassFactory::registerType(std::string type, Creator creator) {
    if (type.empty() || !creator) {
        return false;
    }
    return creators_.emplace(std::move(type), std::move(creator)).second;
}

std::unique_ptr<IRenderPass> RenderPassFactory::create(const std::string_view type) const {
    const auto iterator = creators_.find(std::string{type});
    return iterator == creators_.end() ? nullptr : iterator->second();
}

bool RenderPipeline::build(const RenderPipelineDescriptor& descriptor,
                           const RenderPassFactory& factory, std::string& error) {
    error.clear();
    if (initialized_) {
        error = "Cannot rebuild an initialized render pipeline";
        return false;
    }

    RenderGraphDescriptor graphDescriptor{.resources = descriptor.resources};
    graphDescriptor.passes.reserve(descriptor.passes.size());
    std::unordered_map<std::string, const RenderPassDescriptor*> descriptorsByName;
    descriptorsByName.reserve(descriptor.passes.size());
    for (const RenderPassDescriptor& pass : descriptor.passes) {
        if (pass.name.empty() || pass.type.empty()) {
            error = "Render pass name and type must not be empty";
            return false;
        }
        if (!descriptorsByName.emplace(pass.name, &pass).second) {
            error = "Duplicate render pass name: " + pass.name;
            return false;
        }
        graphDescriptor.passes.push_back(RenderGraphPassDescriptor{
            .name = pass.name,
            .dependsOn = pass.dependsOn,
            .reads = pass.reads,
            .writes = pass.writes,
        });
    }

    CompiledRenderGraph compiledGraph;
    if (!RenderGraphCompiler{}.compile(graphDescriptor, compiledGraph, error)) {
        return false;
    }

    std::vector<Entry> candidate;
    candidate.reserve(compiledGraph.passes.size());
    for (const RenderGraphPassDescriptor& graphPass : compiledGraph.passes) {
        RenderPassDescriptor passDescriptor = *descriptorsByName.at(graphPass.name);
        passDescriptor.dependsOn = graphPass.dependsOn;
        std::unique_ptr<IRenderPass> pass = factory.create(passDescriptor.type);
        if (pass == nullptr) {
            error = "No render pass factory registered for type: " + passDescriptor.type;
            return false;
        }
        candidate.push_back(Entry{passDescriptor, std::move(pass), false});
    }

    entries_ = std::move(candidate);
    return true;
}

bool RenderPipeline::initialize(std::string& error) {
    error.clear();
    if (initialized_) {
        return true;
    }

    for (Entry& entry : entries_) {
        if (!entry.descriptor.enabled) {
            continue;
        }
        const auto inactiveDependency =
            std::find_if(entry.descriptor.dependsOn.begin(), entry.descriptor.dependsOn.end(),
                         [this](const std::string& dependency) {
                             const auto found =
                                 std::find_if(entries_.begin(), entries_.end(),
                                              [&dependency](const Entry& candidate) {
                                                  return candidate.descriptor.name == dependency;
                                              });
                             return found == entries_.end() || !found->active;
                         });
        if (inactiveDependency != entry.descriptor.dependsOn.end()) {
            if (!entry.descriptor.required) {
                continue;
            }
            error = "Required render pass dependency is inactive: " + entry.descriptor.name +
                    " -> " + *inactiveDependency;
            for (auto iterator = entries_.rbegin(); iterator != entries_.rend(); ++iterator) {
                if (iterator->active) {
                    iterator->pass->shutdown();
                    iterator->active = false;
                }
            }
            return false;
        }
        if (entry.pass->initialize()) {
            entry.active = true;
            if (width_ > 0 && height_ > 0) {
                entry.pass->resize(width_, height_);
            }
            continue;
        }
        // initialize 允许在创建部分资源后失败，因此失败对象也必须立即清理。
        entry.pass->shutdown();
        if (!entry.descriptor.required) {
            entry.active = false;
            continue;
        }

        error = "Required render pass failed to initialize: " + entry.descriptor.name;
        for (auto iterator = entries_.rbegin(); iterator != entries_.rend(); ++iterator) {
            if (iterator->active) {
                iterator->pass->shutdown();
                iterator->active = false;
            }
        }
        return false;
    }

    initialized_ = true;
    return true;
}

void RenderPipeline::shutdown() noexcept {
    for (auto iterator = entries_.rbegin(); iterator != entries_.rend(); ++iterator) {
        if (iterator->active) {
            iterator->pass->shutdown();
            iterator->active = false;
        }
    }
    initialized_ = false;
}

void RenderPipeline::resize(const int width, const int height) {
    width_ = std::max(width, 0);
    height_ = std::max(height, 0);
    if (!initialized_ || width_ == 0 || height_ == 0) {
        return;
    }
    for (Entry& entry : entries_) {
        if (entry.active) {
            entry.pass->resize(width_, height_);
        }
    }
}

void RenderPipeline::execute(const RenderPassExecutionContext& context) {
    if (!initialized_) {
        return;
    }
    for (Entry& entry : entries_) {
        if (entry.active) {
            entry.pass->execute(context);
        }
    }
}

bool RenderPipeline::setEnabled(const std::string_view name, const bool enabled) {
    if (initialized_) {
        return false;
    }
    for (Entry& entry : entries_) {
        if (entry.descriptor.name == name) {
            entry.descriptor.enabled = enabled;
            return true;
        }
    }
    return false;
}

bool RenderPipeline::contains(const std::string_view name) const noexcept {
    return std::any_of(entries_.begin(), entries_.end(),
                       [name](const Entry& entry) { return entry.descriptor.name == name; });
}

std::size_t RenderPipeline::size() const noexcept {
    return entries_.size();
}

bool RenderPipeline::initialized() const noexcept {
    return initialized_;
}

} // namespace renderlab
