#include "renderer/pipeline/RenderPipeline.h"

#include <algorithm>
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

    std::vector<Entry> candidate;
    candidate.reserve(descriptor.passes.size());

    for (const RenderPassDescriptor& passDescriptor : descriptor.passes) {
        if (passDescriptor.name.empty() || passDescriptor.type.empty()) {
            error = "Render pass name and type must not be empty";
            return false;
        }
        const bool duplicate =
            std::any_of(candidate.begin(), candidate.end(), [&passDescriptor](const Entry& entry) {
                return entry.descriptor.name == passDescriptor.name;
            });
        if (duplicate) {
            error = "Duplicate render pass name: " + passDescriptor.name;
            return false;
        }

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
