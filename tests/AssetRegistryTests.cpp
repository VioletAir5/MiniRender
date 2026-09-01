#include "assets/AssetRegistry.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("material assets can be created and queried") {
    renderlab::AssetRegistry registry;

    const renderlab::MaterialHandle handle = registry.createMaterial(
        renderlab::MaterialAsset{
            .name = "Orange",
            .baseColorFactor = {0.8F, 0.3F, 0.15F, 1.0F},
        });

    REQUIRE(handle.valid());

    const renderlab::MaterialAsset* material =
        registry.tryGetMaterial(handle);
    REQUIRE(material != nullptr);
    REQUIRE(material->name == "Orange");
    REQUIRE(material->baseColorFactor.r == Catch::Approx(0.8F));
    REQUIRE(material->baseColorFactor.g == Catch::Approx(0.3F));
    REQUIRE(material->baseColorFactor.b == Catch::Approx(0.15F));
    REQUIRE(material->baseColorFactor.a == Catch::Approx(1.0F));
}

TEST_CASE("destroying a material invalidates its old handle") {
    renderlab::AssetRegistry registry;

    const renderlab::MaterialHandle original = registry.createMaterial(
        renderlab::MaterialAsset{.name = "Original"});
    REQUIRE(registry.destroyMaterial(original));
    REQUIRE(registry.tryGetMaterial(original) == nullptr);
    REQUIRE_FALSE(registry.destroyMaterial(original));

    const renderlab::MaterialHandle replacement = registry.createMaterial(
        renderlab::MaterialAsset{.name = "Replacement"});

    REQUIRE(replacement.valid());
    REQUIRE(replacement.index == original.index);
    REQUIRE(replacement.generation != original.generation);
    REQUIRE(registry.tryGetMaterial(replacement) != nullptr);
}

TEST_CASE("invalid material handles return no asset") {
    const renderlab::AssetRegistry registry;

    REQUIRE(registry.tryGetMaterial({}) == nullptr);
    REQUIRE_FALSE(registry.tryGetMaterialView({}).has_value());
}

TEST_CASE("texture stable IDs deduplicate and are removed with the asset") {
    renderlab::AssetRegistry registry;
    constexpr auto TextureId = "test:texture/checker";

    const renderlab::TextureHandle first = registry.createTexture(
        TextureId, renderlab::TextureAsset{.name = "Checker"});
    const renderlab::TextureHandle duplicate = registry.createTexture(
        TextureId, renderlab::TextureAsset{.name = "Ignored Duplicate"});

    REQUIRE(first.valid());
    CHECK(duplicate == first);
    CHECK(registry.findTexture(TextureId) == first);
    REQUIRE(registry.textureId(first).has_value());
    CHECK(*registry.textureId(first) == TextureId);

    REQUIRE(registry.destroyTexture(first));
    CHECK_FALSE(registry.findTexture(TextureId).valid());
    CHECK_FALSE(registry.textureId(first).has_value());

    const renderlab::TextureHandle replacement = registry.createTexture(
        TextureId, renderlab::TextureAsset{.name = "Replacement"});
    CHECK(replacement.valid());
    CHECK(replacement.generation != first.generation);
}
