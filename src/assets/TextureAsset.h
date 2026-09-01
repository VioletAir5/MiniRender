#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace renderlab {

enum class TextureColorSpace {
    Linear,
    SRGB,
};

enum class TextureFormat {
    R8,
    RG8,
    RGB8,
    RGBA8,
};

enum class TextureFilter {
    Nearest,
    Linear,
};

enum class TextureWrap {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
};

struct SamplerDesc {
    TextureFilter minFilter{TextureFilter::Linear};
    TextureFilter magFilter{TextureFilter::Linear};
    TextureWrap wrapU{TextureWrap::Repeat};
    TextureWrap wrapV{TextureWrap::Repeat};
    bool generateMipmaps{true};
};

struct TextureAsset {
    std::string name{"Texture"};
    std::filesystem::path sourcePath;

    std::uint32_t width{0};
    std::uint32_t height{0};
    TextureFormat format{TextureFormat::RGBA8};
    TextureColorSpace colorSpace{TextureColorSpace::SRGB};
    SamplerDesc sampler;
    std::vector<std::byte> pixels;
};




} // namespace renderlab
