#pragma once

#include "assets/AssetHandle.h"

#include <compare>
#include <cstdint>
#include <map>

namespace renderlab {

class AssetRegistry;

// UV 球缓存键；相同的归一化拓扑参数共享同一网格资产。
struct UvSphereTopology {
    std::uint32_t segments{32};
    std::uint32_t rings{16};

    // map 依靠字典序比较查找相同拓扑。
    auto operator<=>(const UvSphereTopology&) const = default;
};

// 按需创建并缓存常用程序化网格，资产所有权仍属于 AssetRegistry。
class ProceduralMeshLibrary {
public:
    // registry 必须比该对象存活更久。
    explicit ProceduralMeshLibrary(AssetRegistry& registry) noexcept;

    // 返回共享的单位立方体资产，失效后会自动重新创建。
    [[nodiscard]] MeshHandle unitCube();
    // 返回共享的单位 XZ 平面资产，失效后会自动重新创建。
    [[nodiscard]] MeshHandle unitPlane();
    // 返回指定拓扑的共享单位 UV 球；输入参数会先归一化。
    [[nodiscard]] MeshHandle uvSphere(std::uint32_t segments = 32,
                                      std::uint32_t rings = 16);

private:
    // 判断缓存句柄是否仍指向注册表中的就绪资产。
    [[nodiscard]] bool isRegistered(MeshHandle handle) const noexcept;

    AssetRegistry& registry_;
    MeshHandle cube_;
    MeshHandle plane_;
    std::map<UvSphereTopology, MeshHandle> spheres_;
};

} // namespace renderlab
