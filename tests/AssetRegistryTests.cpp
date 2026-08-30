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
