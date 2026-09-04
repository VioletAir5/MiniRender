#include "serialization/SceneSerializer.h"

#include "assets/AssetRegistry.h"
#include "scene/SceneDocument.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <optional>
#include <utility>

namespace renderlab {
namespace {

using Json = nlohmann::json;

Json vector3(const glm::vec3& value) {
    return Json::array({value.x, value.y, value.z});
}
Json vector4(const glm::vec4& value) {
    return Json::array({value.x, value.y, value.z, value.w});
}

glm::vec3 readVector3(const Json& value) {
    if (!value.is_array() || value.size() != 3) {
        throw std::runtime_error("Expected a three-component vector");
    }
    return {value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>()};
}

Json serializeEntity(const SceneDocument& scene, const AssetRegistry& assets,
                     const EntityId entity) {
    const EntityMetadata& metadata = *scene.tryGetEntity(entity);
    const TransformComponent& transform = *scene.tryGetTransform(entity);
    Json result{{"id", entity},
                {"parent", metadata.parent},
                {"name", metadata.name},
                {"transform",
                 {{"position", vector3(transform.position)},
                  {"rotation", vector3(transform.rotationDegrees)},
                  {"scale", vector3(transform.scale)}}}};

    if (const auto* renderer = scene.tryGetMeshRenderer(entity); renderer != nullptr) {
        const auto meshId = assets.meshId(renderer->meshAsset);
        const auto materialId = assets.materialId(renderer->materialAsset);
        if (!meshId.has_value() || (renderer->materialAsset.valid() && !materialId.has_value())) {
            throw std::runtime_error("Scene references an asset without a persistent ID");
        }
        result["meshRenderer"] = {{"mesh", *meshId},
                                  {"material", materialId.value_or(std::string{})},
                                  {"visible", renderer->visible},
                                  {"castShadow", renderer->castShadow}};
    }
    if (const auto* camera = scene.tryGetCamera(entity); camera != nullptr) {
        result["camera"] = {{"verticalFovDegrees", camera->verticalFovDegrees},
                            {"nearPlane", camera->nearPlane},
                            {"farPlane", camera->farPlane},
                            {"primary", camera->primary}};
    }
    if (const auto* light = scene.tryGetLight(entity); light != nullptr) {
        result["light"] = {{"type", static_cast<int>(light->type)},
                           {"color", vector3(light->color)},
                           {"intensity", light->intensity},
                           {"range", light->range},
                           {"innerConeDegrees", light->innerConeDegrees},
                           {"outerConeDegrees", light->outerConeDegrees},
                           {"castShadow", light->castShadow},
                           {"shadowTechnique", static_cast<int>(light->shadowTechnique)},
                           {"shadowBias", light->shadowBias},
                           {"shadowDistance", light->shadowDistance}};
    }
    return result;
}

void deserializeEntity(SceneDocument& scene, const AssetRegistry& assets, const Json& value) {
    const EntityId id = value.at("id").get<EntityId>();
    const EntityId parent = value.value("parent", NullEntity);
    if (scene.restoreEntity(id, value.at("name").get<std::string>(), parent) == NullEntity) {
        throw std::runtime_error("Entity ID or parent relationship is invalid");
    }

    const Json& transformJson = value.at("transform");
    TransformComponent transform;
    transform.position = readVector3(transformJson.at("position"));
    transform.rotationDegrees = readVector3(transformJson.at("rotation"));
    transform.scale = readVector3(transformJson.at("scale"));
    (void)scene.setTransform(id, transform);

    if (value.contains("meshRenderer")) {
        const Json& component = value.at("meshRenderer");
        const MeshHandle mesh = assets.findMesh(component.at("mesh").get<std::string>());
        const std::string materialId = component.value("material", std::string{});
        const MaterialHandle material =
            materialId.empty() ? MaterialHandle{} : assets.findMaterial(materialId);
        if (!mesh.valid() || (!materialId.empty() && !material.valid())) {
            throw std::runtime_error("Scene references an unavailable asset");
        }
        scene.addMeshRenderer(id) =
            MeshRendererComponent{.meshAsset = mesh,
                                  .materialAsset = material,
                                  .visible = component.value("visible", true),
                                  .castShadow = component.value("castShadow", true)};
    }
    if (value.contains("camera")) {
        const Json& component = value.at("camera");
        scene.addCamera(id) =
            CameraComponent{.verticalFovDegrees = component.value("verticalFovDegrees", 60.0F),
                            .nearPlane = component.value("nearPlane", 0.1F),
                            .farPlane = component.value("farPlane", 1000.0F),
                            .primary = component.value("primary", false)};
    }
    if (value.contains("light")) {
        const Json& component = value.at("light");
        scene.addLight(id) =
            LightComponent{.type = static_cast<LightType>(component.value("type", 0)),
                           .color = readVector3(component.at("color")),
                           .intensity = component.value("intensity", 1.0F),
                           .range = component.value("range", 10.0F),
                           .innerConeDegrees = component.value("innerConeDegrees", 20.0F),
                           .outerConeDegrees = component.value("outerConeDegrees", 30.0F),
                           .castShadow = component.value("castShadow", true),
                           .shadowTechnique = static_cast<ShadowTechnique>(component.value(
                               "shadowTechnique", static_cast<int>(ShadowTechnique::Pcf))),
                           .shadowBias = component.value("shadowBias", 0.0015F),
                           .shadowDistance = component.value("shadowDistance", 50.0F)};
    }
}

} // namespace

SceneIoResult SceneSerializer::save(const SceneDocument& scene, const AssetRegistry& assets,
                                    const std::filesystem::path& path) {
    try {
        Json document{{"format", "RenderLabScene"}, {"version", 1}, {"entities", Json::array()}};
        for (const auto& [id, metadata] : scene.entities()) {
            (void)metadata;
            document["entities"].push_back(serializeEntity(scene, assets, id));
        }
        std::ofstream output(path);
        if (!output)
            return {false, "Cannot open the scene file for writing"};
        output << document.dump(2);
        return {true, {}};
    } catch (const std::exception& error) {
        return {false, error.what()};
    }
}

SceneIoResult SceneSerializer::load(SceneDocument& destination, const AssetRegistry& assets,
                                    const std::filesystem::path& path) {
    try {
        std::ifstream input(path);
        if (!input)
            return {false, "Cannot open the scene file"};
        const Json document = Json::parse(input);
        if (document.value("format", std::string{}) != "RenderLabScene" ||
            document.value("version", 0) != 1) {
            return {false, "Unsupported scene format or version"};
        }

        SceneDocument loaded;
        std::vector<Json> pending = document.at("entities").get<std::vector<Json>>();
        while (!pending.empty()) {
            bool progressed = false;
            for (auto iterator = pending.begin(); iterator != pending.end();) {
                const EntityId parent = iterator->value("parent", NullEntity);
                if (parent == NullEntity || loaded.contains(parent)) {
                    deserializeEntity(loaded, assets, *iterator);
                    iterator = pending.erase(iterator);
                    progressed = true;
                } else {
                    ++iterator;
                }
            }
            if (!progressed)
                return {false, "Scene contains missing parents or a hierarchy cycle"};
        }
        destination = std::move(loaded);
        return {true, {}};
    } catch (const std::exception& error) {
        return {false, error.what()};
    }
}

} // namespace renderlab
