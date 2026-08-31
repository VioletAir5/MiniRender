#include "assets/ProceduralMeshLibrary.h"

#include "assets/AssetId.h"
#include "assets/AssetRegistry.h"
#include "assets/PrimitiveFactory.h"

#include <algorithm>
#include <string>
#include <utility>

namespace renderlab {
namespace {

// 将单个程序化 Primitive 包装为注册表可接管的 MeshAsset。
MeshAsset makeMeshAsset(std::string name, MeshPrimitive primitive) {
    MeshAsset mesh;
    mesh.name = std::move(name);
    mesh.primitives.push_back(std::move(primitive));
    return mesh;
}

} // namespace

ProceduralMeshLibrary::ProceduralMeshLibrary(
    AssetRegistry& registry) noexcept
    : registry_(registry) {}

MeshHandle ProceduralMeshLibrary::unitCube() {
    if (isRegistered(cube_)) {
        return cube_;
    }

    cube_ = registry_.createMesh(std::string{asset_ids::UnitCube}, makeMeshAsset(
        "Unit Cube", primitive_factory::createCube(1.0F)));
    return cube_;
}

MeshHandle ProceduralMeshLibrary::unitPlane() {
    if (isRegistered(plane_)) {
        return plane_;
    }

    plane_ = registry_.createMesh(std::string{asset_ids::UnitPlane}, makeMeshAsset(
        "Unit Plane", primitive_factory::createPlane(1.0F, 1.0F)));
    return plane_;
}

MeshHandle ProceduralMeshLibrary::uvSphere(const std::uint32_t segments,
                                           const std::uint32_t rings) {
    // 缓存键必须使用与工厂一致的最小细分限制。
    const UvSphereTopology topology{
        .segments = std::max(segments, 3U),
        .rings = std::max(rings, 2U),
    };

    const auto existing = spheres_.find(topology);
    if (existing != spheres_.end()) {
        if (isRegistered(existing->second)) {
            return existing->second;
        }
        // 资产可能被外部销毁；移除旧句柄后按需重新创建。
        spheres_.erase(existing);
    }

    const std::string name = "Unit UV Sphere " +
                             std::to_string(topology.segments) + "x" +
                             std::to_string(topology.rings);
    const std::string id = "builtin:mesh/uv-sphere/" +
                           std::to_string(topology.segments) + "x" +
                           std::to_string(topology.rings);
    const MeshHandle handle = registry_.createMesh(id, makeMeshAsset(
        name,
        primitive_factory::createUvSphere(
            1.0F, topology.segments, topology.rings)));

    if (handle.valid()) {
        spheres_.emplace(topology, handle);
    }
    return handle;
}

bool ProceduralMeshLibrary::isRegistered(
    const MeshHandle handle) const noexcept {
    return registry_.tryGetMesh(handle) != nullptr;
}

} // namespace renderlab
