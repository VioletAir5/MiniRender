#include "assets/AssetRegistry.h"
#include "importers/GltfImporter.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

class TemporaryGltf final {
public:
    TemporaryGltf() {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        directory = std::filesystem::temp_directory_path() /
                    ("renderlab-gltf-test-" + std::to_string(suffix));
        std::filesystem::create_directories(directory);

        const std::array<float, 9> positions{
            0.0F, 0.0F, 0.0F,
            1.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F,
        };
        const std::array<float, 9> normals{
            0.0F, 0.0F, 1.0F,
            0.0F, 0.0F, 1.0F,
            0.0F, 0.0F, 1.0F,
        };
        const std::array<float, 6> texCoords{
            0.0F, 0.0F,
            1.0F, 0.0F,
            0.0F, 1.0F,
        };
        const std::array<std::uint16_t, 3> indices{0, 1, 2};
        std::ofstream binary{directory / "triangle.bin", std::ios::binary};
        binary.write(reinterpret_cast<const char*>(positions.data()), sizeof(positions));
        binary.write(reinterpret_cast<const char*>(normals.data()), sizeof(normals));
        binary.write(reinterpret_cast<const char*>(texCoords.data()), sizeof(texCoords));
        binary.write(reinterpret_cast<const char*>(indices.data()), sizeof(indices));

        std::ofstream json{directory / "triangle.gltf"};
        const std::array<std::uint8_t, 70> png{
            0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,
            0x49,0x48,0x44,0x52,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,
            0x08,0x06,0x00,0x00,0x00,0x1F,0x15,0xC4,0x89,0x00,0x00,0x00,
            0x0D,0x49,0x44,0x41,0x54,0x08,0xD7,0x63,0xF8,0xCF,0xC0,0xF0,
            0x1F,0x00,0x05,0x00,0x01,0xFF,0x89,0x99,0x3D,0x1D,0x00,0x00,
            0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82};
        std::ofstream image{directory / "pixel.png", std::ios::binary};
        image.write(reinterpret_cast<const char*>(png.data()),
                    static_cast<std::streamsize>(png.size()));

        json << R"({
  "asset": {"version": "2.0"},
  "buffers": [{"uri": "triangle.bin", "byteLength": 102}],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 36},
    {"buffer": 0, "byteOffset": 72, "byteLength": 24},
    {"buffer": 0, "byteOffset": 96, "byteLength": 6}
  ],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC2"},
    {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"}
  ],
  "materials": [{"name": "Red", "pbrMetallicRoughness": {
    "baseColorFactor": [0.8, 0.2, 0.1, 1.0],
    "metallicFactor": 0.25, "roughnessFactor": 0.75,
    "baseColorTexture": {"index": 0},
    "metallicRoughnessTexture": {"index": 0}
  }, "normalTexture": {"index": 0, "scale": 0.7}}],
  "meshes": [{"name": "Triangle", "primitives": [{
    "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
    "indices": 3, "material": 0
  }]}],
  "images": [{"uri": "pixel.png", "name": "Pixel"}],
  "textures": [{"source": 0}],
  "nodes": [{"name": "Moved Triangle", "mesh": 0, "translation": [1, 2, 3]}],
  "scenes": [{"nodes": [0]}],
  "scene": 0
})";
    }

    ~TemporaryGltf() {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }

    std::filesystem::path directory;
};

} // namespace

TEST_CASE("glTF importer builds hierarchy mesh and material assets") {
    TemporaryGltf fixture;
    renderlab::AssetRegistry registry;
    renderlab::GltfImporter importer{registry};

    const renderlab::GltfImportResult result =
        importer.import(fixture.directory / "triangle.gltf");

    REQUIRE(result);
    REQUIRE(result.meshCount == 1);
    REQUIRE(result.materialCount == 1);
    REQUIRE(result.snapshot->children.size() == 1);
    // 同一图像分别作为 sRGB Base Color 和线性 MR 数据时会生成两个纹理资产。
    REQUIRE(result.textureCount == 2);
    const auto& node = result.snapshot->children.front();
    REQUIRE(node.name == "Moved Triangle");
    REQUIRE(node.transform.position.x == Catch::Approx(1.0F));
    REQUIRE(node.transform.position.y == Catch::Approx(2.0F));
    REQUIRE(node.transform.position.z == Catch::Approx(3.0F));
    REQUIRE(node.meshRenderer.has_value());

    const renderlab::MeshAsset* mesh =
        registry.tryGetMesh(node.meshRenderer->meshAsset);
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->primitives.size() == 1);
    REQUIRE(mesh->primitives.front().vertices.size() == 3);
    REQUIRE(mesh->primitives.front().indices ==
            std::vector<std::uint32_t>{0, 1, 2});
    const renderlab::Vertex& firstVertex =
        mesh->primitives.front().vertices.front();
    REQUIRE(firstVertex.tangent.x == Catch::Approx(1.0F));
    REQUIRE(firstVertex.tangent.y == Catch::Approx(0.0F));
    REQUIRE(firstVertex.tangent.z == Catch::Approx(0.0F));
    REQUIRE(firstVertex.tangent.w == Catch::Approx(1.0F));

    const renderlab::MaterialAsset* material =
        registry.tryGetMaterial(mesh->primitives.front().defaultMaterial);
    REQUIRE(material != nullptr);
    REQUIRE(material->baseColorFactor.r == Catch::Approx(0.8F));
    REQUIRE(material->metallicFactor == Catch::Approx(0.25F));
    REQUIRE(material->roughnessFactor == Catch::Approx(0.75F));
    REQUIRE(material->baseColorTexture.has_value());
    REQUIRE(material->metallicRoughnessTexture.has_value());
    REQUIRE(material->normalTexture.has_value());
    REQUIRE(material->normalScale == Catch::Approx(0.7F));
    REQUIRE(material->normalTexture->texture ==
            material->metallicRoughnessTexture->texture);
    const renderlab::TextureAsset* texture =
        registry.tryGetTexture(material->baseColorTexture->texture);
    REQUIRE(texture != nullptr);
    REQUIRE(texture->width == 1);
    REQUIRE(texture->height == 1);

    const renderlab::TextureAsset* metallicRoughnessTexture =
        registry.tryGetTexture(material->metallicRoughnessTexture->texture);
    REQUIRE(metallicRoughnessTexture != nullptr);
    REQUIRE(metallicRoughnessTexture->colorSpace ==
            renderlab::TextureColorSpace::Linear);

}
TEST_CASE("glTF importer reports a missing source file") {
    renderlab::AssetRegistry registry;
    renderlab::GltfImporter importer{registry};
    const auto result = importer.import(
        std::filesystem::temp_directory_path() / "renderlab-missing-model.glb");
    REQUIRE_FALSE(result);
    REQUIRE_FALSE(result.error.empty());
}
