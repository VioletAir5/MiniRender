#pragma once

#include <filesystem>
#include <string>

namespace renderlab {

// 描述与图形 API 无关的一个顶点/片元 Shader 源码组合。
struct ShaderAsset {
    std::string name{"Shader"};
    // 路径相对于 ShaderLibrary 的资源根目录，禁止使用绝对路径和父目录跳转。
    std::filesystem::path vertexSource;
    std::filesystem::path fragmentSource;
};

// ShaderLibrary 加载后的源码快照；后端可据此创建自己的 GPU 程序对象。
struct ShaderSourceBundle {
    std::string vertexSource;
    std::string fragmentSource;
};

} // namespace renderlab
