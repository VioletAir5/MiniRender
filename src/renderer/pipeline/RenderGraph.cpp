#include "renderer/pipeline/RenderGraph.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <queue>
#include <unordered_map>
#include <utility>

namespace renderlab {
namespace {

bool validResourceSize(const RenderResourceDescriptor& resource) {
    if (resource.sizeMode == RenderResourceSizeMode::Viewport) {
        return resource.widthScale > 0.0F && resource.heightScale > 0.0F;
    }
    return resource.width > 0 && resource.height > 0;
}

void addDependency(RenderGraphPassDescriptor& pass, const std::string& dependency) {
    if (dependency != pass.name && std::find(pass.dependsOn.begin(), pass.dependsOn.end(),
                                             dependency) == pass.dependsOn.end()) {
        pass.dependsOn.push_back(dependency);
    }
}

} // namespace

bool RenderGraphCompiler::compile(const RenderGraphDescriptor& descriptor,
                                  CompiledRenderGraph& result, std::string& error) const {
    error.clear();
    result.resources.clear();
    result.passes.clear();

    std::unordered_map<std::string, const RenderResourceDescriptor*> resources;
    resources.reserve(descriptor.resources.size());
    for (const RenderResourceDescriptor& resource : descriptor.resources) {
        if (resource.name.empty()) {
            error = "Render resource name must not be empty";
            return false;
        }
        if (!validResourceSize(resource)) {
            error = "Invalid render resource size: " + resource.name;
            return false;
        }
        if (!resources.emplace(resource.name, &resource).second) {
            error = "Duplicate render resource name: " + resource.name;
            return false;
        }
    }

    std::unordered_map<std::string, std::size_t> passIndices;
    passIndices.reserve(descriptor.passes.size());
    for (std::size_t index = 0; index < descriptor.passes.size(); ++index) {
        const auto& pass = descriptor.passes[index];
        if (pass.name.empty()) {
            error = "Render graph pass name must not be empty";
            return false;
        }
        if (!passIndices.emplace(pass.name, index).second) {
            error = "Duplicate render graph pass name: " + pass.name;
            return false;
        }
    }

    std::vector<RenderGraphPassDescriptor> normalized = descriptor.passes;
    std::unordered_map<std::string, std::size_t> producers;
    for (std::size_t passIndex = 0; passIndex < normalized.size(); ++passIndex) {
        const auto& pass = normalized[passIndex];
        for (const std::string& resourceName : pass.writes) {
            const auto resource = resources.find(resourceName);
            if (resource == resources.end()) {
                error = "Pass '" + pass.name + "' writes unknown resource: " + resourceName;
                return false;
            }
            if (!resource->second->external && !producers.emplace(resourceName, passIndex).second) {
                error = "Transient render resource has multiple producers: " + resourceName;
                return false;
            }
        }
    }

    for (RenderGraphPassDescriptor& pass : normalized) {
        const std::vector<std::string> declaredDependencies = std::move(pass.dependsOn);
        pass.dependsOn.clear();
        for (const std::string& dependency : declaredDependencies) {
            if (!passIndices.contains(dependency)) {
                error = "Pass '" + pass.name + "' depends on unknown pass: " + dependency;
                return false;
            }
            if (dependency == pass.name) {
                error = "Render graph pass cannot depend on itself: " + pass.name;
                return false;
            }
            addDependency(pass, dependency);
        }
        for (const std::string& resourceName : pass.reads) {
            const auto resource = resources.find(resourceName);
            if (resource == resources.end()) {
                error = "Pass '" + pass.name + "' reads unknown resource: " + resourceName;
                return false;
            }
            if (resource->second->external) {
                continue;
            }
            const auto producer = producers.find(resourceName);
            if (producer == producers.end()) {
                error = "Transient render resource has no producer: " + resourceName;
                return false;
            }
            if (normalized[producer->second].name == pass.name) {
                error = "Pass '" + pass.name +
                        "' cannot read its own transient output: " + resourceName;
                return false;
            }
            addDependency(pass, normalized[producer->second].name);
        }
    }

    std::vector<std::vector<std::size_t>> outgoing(normalized.size());
    std::vector<std::size_t> indegree(normalized.size(), 0);
    for (std::size_t passIndex = 0; passIndex < normalized.size(); ++passIndex) {
        for (const std::string& dependency : normalized[passIndex].dependsOn) {
            const std::size_t dependencyIndex = passIndices.at(dependency);
            outgoing[dependencyIndex].push_back(passIndex);
            ++indegree[passIndex];
        }
    }

    std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>> ready;
    for (std::size_t index = 0; index < indegree.size(); ++index) {
        if (indegree[index] == 0) {
            ready.push(index);
        }
    }

    result.passes.reserve(normalized.size());
    while (!ready.empty()) {
        const std::size_t index = ready.top();
        ready.pop();
        result.passes.push_back(std::move(normalized[index]));
        for (const std::size_t dependent : outgoing[index]) {
            if (--indegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (result.passes.size() != descriptor.passes.size()) {
        result.resources.clear();
        result.passes.clear();
        error = "Render graph contains a dependency cycle";
        return false;
    }
    result.resources = descriptor.resources;
    return true;
}

} // namespace renderlab
