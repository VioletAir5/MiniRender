#include "importers/GltfImporter.h"

#include "assets/AssetRegistry.h"
#include "assets/MeshTangents.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cmath>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace renderlab {
namespace {

// 同一个 glTF texture 可因用途不同分别创建 sRGB 和线性版本。
struct TextureKey {
    std::size_t index{0};
    TextureColorSpace colorSpace{TextureColorSpace::Linear};

    bool operator==(const TextureKey&) const = default;
};

struct TextureKeyHash {
    std::size_t operator()(const TextureKey& key) const noexcept {
        return key.index * 2U +
               (key.colorSpace == TextureColorSpace::SRGB ? 1U : 0U);
    }
};

std::string normalizedSourceId(const std::filesystem::path& path) {
    const auto text = std::filesystem::absolute(path).lexically_normal().generic_u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

std::string namedOr(const std::string_view name, const std::string& fallback) {
    return name.empty() ? fallback : std::string{name};
}

TextureWrap convertWrap(const fastgltf::Wrap wrap) noexcept {
    switch (wrap) {
    case fastgltf::Wrap::ClampToEdge: return TextureWrap::ClampToEdge;
    case fastgltf::Wrap::MirroredRepeat: return TextureWrap::MirroredRepeat;
    case fastgltf::Wrap::Repeat: return TextureWrap::Repeat;
    }
    return TextureWrap::Repeat;
}

SamplerDesc convertSampler(const fastgltf::Asset& asset,
                           const fastgltf::Texture& texture) {
    SamplerDesc result;
    if (!texture.samplerIndex.has_value()) {
        return result;
    }
    const fastgltf::Sampler& source = asset.samplers[*texture.samplerIndex];
    result.wrapU = convertWrap(source.wrapS);
    result.wrapV = convertWrap(source.wrapT);
    if (source.magFilter.has_value()) {
        result.magFilter = *source.magFilter == fastgltf::Filter::Nearest
                               ? TextureFilter::Nearest : TextureFilter::Linear;
    }
    if (source.minFilter.has_value()) {
        const auto filter = *source.minFilter;
        result.minFilter = filter == fastgltf::Filter::Nearest ||
                                   filter == fastgltf::Filter::NearestMipMapNearest ||
                                   filter == fastgltf::Filter::NearestMipMapLinear
                               ? TextureFilter::Nearest : TextureFilter::Linear;
        result.generateMipmaps = filter != fastgltf::Filter::Nearest &&
                                 filter != fastgltf::Filter::Linear;
    }
    return result;
}

std::span<const std::byte> imageBytes(const fastgltf::Asset& asset,
                                      const fastgltf::Image& image) {
    return std::visit(fastgltf::visitor{
        [&](const fastgltf::sources::BufferView& source) {
            const auto bytes = fastgltf::DefaultBufferDataAdapter{}(
                asset, source.bufferViewIndex);
            return std::span<const std::byte>{bytes.data(), bytes.size()};
        },
        [](const fastgltf::sources::Array& source) {
            return std::span<const std::byte>{source.bytes.data(),
                                              source.bytes.size_bytes()};
        },
        [](const fastgltf::sources::Vector& source) {
            return std::span<const std::byte>{source.bytes.data(),
                                              source.bytes.size()};
        },
        [](const fastgltf::sources::ByteView& source) {
            return std::span<const std::byte>{source.bytes.data(),
                                              source.bytes.size()};
        },
        [](const auto&) { return std::span<const std::byte>{}; },
    }, image.data);
}

glm::vec3 quaternionEulerDegrees(const glm::quat& q) noexcept {
    const float sinXCosY = 2.0F * (q.w * q.x + q.y * q.z);
    const float cosXCosY = 1.0F - 2.0F * (q.x * q.x + q.y * q.y);
    const float sinY = 2.0F * (q.w * q.y - q.z * q.x);
    const float sinZCosY = 2.0F * (q.w * q.z + q.x * q.y);
    const float cosZCosY = 1.0F - 2.0F * (q.y * q.y + q.z * q.z);
    const glm::vec3 radians{
        std::atan2(sinXCosY, cosXCosY),
        std::asin(glm::clamp(sinY, -1.0F, 1.0F)),
        std::atan2(sinZCosY, cosZCosY),
    };
    return glm::degrees(radians);
}

TransformComponent convertTransform(const fastgltf::Node& node) {
    const auto& trs = std::get<fastgltf::TRS>(node.transform);
    const glm::quat rotation{trs.rotation[3], trs.rotation[0],
                             trs.rotation[1], trs.rotation[2]};

    return TransformComponent{
        .position = {trs.translation[0], trs.translation[1], trs.translation[2]},
        .rotationDegrees = quaternionEulerDegrees(rotation),
        .scale = {trs.scale[0], trs.scale[1], trs.scale[2]},
    };
}

class ImportContext {
public:
    ImportContext(AssetRegistry& registry, fastgltf::Asset& asset,
                  std::string sourceId)
        : registry_(registry), asset_(asset), sourceId_(std::move(sourceId)),
          meshes_(asset.meshes.size()), materials_(asset.materials.size()) {}

    GltfImportResult build(const std::filesystem::path& path) {
        GltfImportResult result;
        EntitySnapshot root;
        root.name = path.stem().string();

        if (asset_.scenes.empty()) {
            fail("glTF does not contain a scene");
            result.error = error_;
            return result;
        }
        const std::size_t sceneIndex = asset_.defaultScene.value_or(0U);
        if (sceneIndex >= asset_.scenes.size()) {
            fail("glTF default scene index is invalid");
            result.error = error_;
            return result;
        }

        for (const std::size_t nodeIndex : asset_.scenes[sceneIndex].nodeIndices) {
            auto child = importNode(nodeIndex);
            if (!child.has_value()) {
                result.error = error_;
                return result;
            }
            root.children.push_back(std::move(*child));
        }
        result.snapshot = std::move(root);
        result.meshCount = importedMeshes_;
        result.materialCount = importedMaterials_;
        result.textureCount = importedTextures_;
        return result;
    }

private:
    void fail(std::string message) {
        if (error_.empty()) error_ = std::move(message);
    }

    TextureHandle importTexture(const std::size_t textureIndex,
                                const TextureColorSpace colorSpace) {
        const TextureKey key{textureIndex, colorSpace};
        if (const auto it = textures_.find(key); it != textures_.end()) return it->second;
        if (textureIndex >= asset_.textures.size()) {
            fail("Material references an invalid texture");
            return {};
        }
        const fastgltf::Texture& source = asset_.textures[textureIndex];
        if (!source.imageIndex.has_value() || *source.imageIndex >= asset_.images.size()) {
            fail("Texture does not reference a supported image");
            return {};
        }
        const auto encoded = imageBytes(asset_, asset_.images[*source.imageIndex]);
        if (encoded.empty() || encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            fail("Texture image data is missing or too large");
            return {};
        }

        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        stbi_uc* decoded = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(encoded.data()),
            static_cast<int>(encoded.size()), &width, &height, &sourceChannels, 4);
        if (decoded == nullptr || width <= 0 || height <= 0) {
            fail(std::string{"Failed to decode glTF image: "} + stbi_failure_reason());
            stbi_image_free(decoded);
            return {};
        }

        const std::size_t byteCount = static_cast<std::size_t>(width) *
                                      static_cast<std::size_t>(height) * 4U;
        TextureAsset texture;
        texture.name = namedOr(source.name, "Texture " + std::to_string(textureIndex));
        texture.width = static_cast<std::uint32_t>(width);
        texture.height = static_cast<std::uint32_t>(height);
        texture.format = TextureFormat::RGBA8;
        texture.colorSpace = colorSpace;
        texture.sampler = convertSampler(asset_, source);
        texture.pixels.resize(byteCount);
        std::memcpy(texture.pixels.data(), decoded, byteCount);
        stbi_image_free(decoded);

        const std::string id = sourceId_ + "#texture/" + std::to_string(textureIndex) +
                               (colorSpace == TextureColorSpace::SRGB ? "/srgb" : "/linear");
        const TextureHandle handle = registry_.createTexture(id, std::move(texture));
        if (!handle.valid()) {
            fail("Texture asset registry is full");
            return {};
        }
        textures_.emplace(key, handle);
        ++importedTextures_;
        return handle;
    }

    template<typename OptionalTextureInfo>
    std::optional<TextureBinding> importBinding(
        const OptionalTextureInfo& info,
        const TextureColorSpace colorSpace) {
        if (!info.has_value()) return std::nullopt;
        const TextureHandle handle = importTexture(info->textureIndex, colorSpace);
        if (!handle.valid()) return std::nullopt;
        TextureBinding binding{.texture = handle,
                               .texCoordSet = static_cast<std::uint32_t>(info->texCoordIndex)};
        if (info->transform != nullptr) {
            binding.offset = {static_cast<float>(info->transform->uvOffset[0]),
                              static_cast<float>(info->transform->uvOffset[1])};
            binding.scale = {static_cast<float>(info->transform->uvScale[0]),
                             static_cast<float>(info->transform->uvScale[1])};
            binding.rotationRadians = static_cast<float>(info->transform->rotation);
            if (info->transform->texCoordIndex.has_value()) {
                binding.texCoordSet = static_cast<std::uint32_t>(*info->transform->texCoordIndex);
            }
        }
        return binding;
    }

    MaterialHandle importMaterial(const std::size_t index) {
        if (index >= asset_.materials.size()) return {};
        if (materials_[index].valid()) return materials_[index];
        const fastgltf::Material& source = asset_.materials[index];
        MaterialAsset material;
        material.name = namedOr(source.name, "Material " + std::to_string(index));
        material.baseColorFactor = {
            static_cast<float>(source.pbrData.baseColorFactor[0]),
            static_cast<float>(source.pbrData.baseColorFactor[1]),
            static_cast<float>(source.pbrData.baseColorFactor[2]),
            static_cast<float>(source.pbrData.baseColorFactor[3])};
        material.metallicFactor = static_cast<float>(source.pbrData.metallicFactor);
        material.roughnessFactor = static_cast<float>(source.pbrData.roughnessFactor);
        material.baseColorTexture = importBinding(source.pbrData.baseColorTexture, TextureColorSpace::SRGB);
        material.metallicRoughnessTexture = importBinding(
            source.pbrData.metallicRoughnessTexture,
            TextureColorSpace::Linear);
        material.normalTexture = importBinding(source.normalTexture, TextureColorSpace::Linear);
        material.occlusionTexture = importBinding(source.occlusionTexture, TextureColorSpace::Linear);
        material.emissiveTexture = importBinding(source.emissiveTexture, TextureColorSpace::SRGB);
        material.normalScale = source.normalTexture.has_value()
                                   ? static_cast<float>(source.normalTexture->scale)
                                   : 1.0F;
        material.occlusionStrength = source.occlusionTexture.has_value()
                                         ? static_cast<float>(
                                               source.occlusionTexture->strength)
                                         : 1.0F;
        material.emissiveFactor = {
            static_cast<float>(source.emissiveFactor[0] * source.emissiveStrength),
            static_cast<float>(source.emissiveFactor[1] * source.emissiveStrength),
            static_cast<float>(source.emissiveFactor[2] * source.emissiveStrength)};
        material.alphaMode = source.alphaMode == fastgltf::AlphaMode::Mask
                                 ? AlphaMode::Mask
                                 : source.alphaMode == fastgltf::AlphaMode::Blend
                                       ? AlphaMode::Blend : AlphaMode::Opaque;
        material.alphaCutoff = static_cast<float>(source.alphaCutoff);
        material.doubleSided = source.doubleSided;
        material.unlit = source.unlit;
        if (!error_.empty()) return {};

        const MaterialHandle handle = registry_.createMaterial(
            sourceId_ + "#material/" + std::to_string(index), std::move(material));
        if (!handle.valid()) {
            fail("Material asset registry is full");
            return {};
        }
        materials_[index] = handle;
        ++importedMaterials_;
        return handle;
    }

    MeshHandle importMesh(const std::size_t index) {
        if (index >= asset_.meshes.size()) return {};
        if (meshes_[index].valid()) return meshes_[index];
        const fastgltf::Mesh& source = asset_.meshes[index];
        MeshAsset mesh;
        mesh.name = namedOr(source.name, "Mesh " + std::to_string(index));

        for (const fastgltf::Primitive& primitiveSource : source.primitives) {
            if (primitiveSource.type != fastgltf::PrimitiveType::Triangles) {
                fail("Only triangle glTF primitives are supported");
                return {};
            }
            const auto positionIt = primitiveSource.findAttribute("POSITION");
            if (positionIt == primitiveSource.attributes.end()) {
                fail("glTF primitive is missing POSITION");
                return {};
            }
            const auto& positionAccessor = asset_.accessors[positionIt->accessorIndex];
            MeshPrimitive primitive;
            primitive.vertices.resize(positionAccessor.count);
            std::size_t vertexIndex = 0;
            for (const auto position : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset_, positionAccessor)) {
                primitive.vertices[vertexIndex++].position = {position[0], position[1], position[2]};
            }
            const auto normalIt = primitiveSource.findAttribute("NORMAL");
            if (normalIt != primitiveSource.attributes.end()) {
                vertexIndex = 0;
                const auto& normalAccessor =
                    asset_.accessors[normalIt->accessorIndex];
                for (const auto normal :
                     fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                         asset_, normalAccessor)) {
                    primitive.vertices[vertexIndex++].normal =
                        {normal[0], normal[1], normal[2]};
                }
            }
            const auto uvIt = primitiveSource.findAttribute("TEXCOORD_0");
            if (uvIt != primitiveSource.attributes.end()) {
                vertexIndex = 0;
                const auto& uvAccessor = asset_.accessors[uvIt->accessorIndex];
                for (const auto uv :
                     fastgltf::iterateAccessor<fastgltf::math::fvec2>(
                         asset_, uvAccessor)) {
                    primitive.vertices[vertexIndex++].texCoord = {uv[0], uv[1]};
                }
            }
            bool hasImportedTangents = false;
            const auto tangentIt = primitiveSource.findAttribute("TANGENT");
            if (tangentIt != primitiveSource.attributes.end()) {
                vertexIndex = 0;
                const auto& tangentAccessor =
                    asset_.accessors[tangentIt->accessorIndex];
                for (const auto tangent :
                     fastgltf::iterateAccessor<fastgltf::math::fvec4>(
                         asset_, tangentAccessor)) {
                    primitive.vertices[vertexIndex++].tangent =
                        {tangent[0], tangent[1], tangent[2], tangent[3]};
                }
                hasImportedTangents = true;
            }
            if (primitiveSource.indicesAccessor.has_value()) {
                const auto& accessor = asset_.accessors[*primitiveSource.indicesAccessor];
                primitive.indices.reserve(accessor.count);
                for (const auto value : fastgltf::iterateAccessor<std::uint32_t>(asset_, accessor)) {
                    primitive.indices.push_back(value);
                }
            } else {
                primitive.indices.resize(primitive.vertices.size());
                for (std::size_t i = 0; i < primitive.indices.size(); ++i) {
                    primitive.indices[i] = static_cast<std::uint32_t>(i);
                }
            }
            if (!hasImportedTangents) {
                // glTF 允许省略 TANGENT；用现有位置、法线和 UV 构造第一版切线空间。
                generateTangents(primitive);
            }
            if (primitiveSource.materialIndex.has_value()) {
                primitive.defaultMaterial = importMaterial(*primitiveSource.materialIndex);
                if (!primitive.defaultMaterial.valid() && !error_.empty()) return {};
            }
            mesh.primitives.push_back(std::move(primitive));
        }

        const MeshHandle handle = registry_.createMesh(
            sourceId_ + "#mesh/" + std::to_string(index), std::move(mesh));
        if (!handle.valid()) {
            fail("Mesh asset registry is full");
            return {};
        }
        meshes_[index] = handle;
        ++importedMeshes_;
        return handle;
    }

    std::optional<EntitySnapshot> importNode(const std::size_t index) {
        if (index >= asset_.nodes.size()) {
            fail("Scene references an invalid node");
            return std::nullopt;
        }
        const fastgltf::Node& source = asset_.nodes[index];
        EntitySnapshot node;
        node.name = namedOr(source.name, "Node " + std::to_string(index));
        node.transform = convertTransform(source);
        if (source.meshIndex.has_value()) {
            const MeshHandle mesh = importMesh(*source.meshIndex);
            if (!mesh.valid()) return std::nullopt;
            node.meshRenderer = MeshRendererComponent{.meshAsset = mesh};
        }
        for (const std::size_t childIndex : source.children) {
            auto child = importNode(childIndex);
            if (!child.has_value()) return std::nullopt;
            node.children.push_back(std::move(*child));
        }
        return node;
    }

    AssetRegistry& registry_;
    fastgltf::Asset& asset_;
    std::string sourceId_;
    std::vector<MeshHandle> meshes_;
    std::vector<MaterialHandle> materials_;
    std::unordered_map<TextureKey, TextureHandle, TextureKeyHash> textures_;
    std::string error_;
    std::size_t importedMeshes_{0};
    std::size_t importedMaterials_{0};
    std::size_t importedTextures_{0};
};

} // namespace

GltfImporter::GltfImporter(AssetRegistry& registry) noexcept : registry_(registry) {}

GltfImportResult GltfImporter::import(const std::filesystem::path& path) {
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (!data) {
        return {.error = std::string{fastgltf::getErrorMessage(data.error())}};
    }
    fastgltf::Parser parser{fastgltf::Extensions::KHR_texture_transform |
                            fastgltf::Extensions::KHR_materials_unlit |
                            fastgltf::Extensions::KHR_materials_emissive_strength};
    constexpr auto options = fastgltf::Options::LoadExternalBuffers |
                             fastgltf::Options::LoadExternalImages |
                             fastgltf::Options::GenerateMeshIndices |
                             fastgltf::Options::DecomposeNodeMatrices;
    auto loaded = parser.loadGltf(data.get(), path.parent_path(), options);
    if (!loaded) {
        return {.error = std::string{fastgltf::getErrorMessage(loaded.error())}};
    }
    ImportContext context{registry_, loaded.get(), normalizedSourceId(path)};
    return context.build(path);
}

} // namespace renderlab
