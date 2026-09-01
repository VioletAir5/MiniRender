#include "renderer/backends/opengl/OpenGLTexture.h"

#include <cstddef>
#include <limits>
#include <utility>

namespace renderlab {

OpenGLTexture::~OpenGLTexture() {
    shutdown();
}

OpenGLTexture::OpenGLTexture(OpenGLTexture&& other) noexcept
    : texture_(std::exchange(other.texture_, 0)) {
}

OpenGLTexture& OpenGLTexture::operator=(OpenGLTexture&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    shutdown();
    texture_ = std::exchange(other.texture_, 0);
    return *this;
}

bool OpenGLTexture::upload(const TextureAsset& asset) {
    const std::size_t channels =
        asset.format == TextureFormat::R8
            ? 1U
            : asset.format == TextureFormat::RG8
                  ? 2U
                  : asset.format == TextureFormat::RGB8 ? 3U : 4U;
    const std::size_t width = asset.width;
    const std::size_t height = asset.height;
    if (width == 0U || height == 0U ||
        width > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        height > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        width > std::numeric_limits<std::size_t>::max() / height / channels) {
        return false;
    }
    if (asset.pixels.size() != width * height * channels) {
        return false;
    }

    if (texture_ != 0) {
        shutdown();
    }
    GLint wrapU = (asset.sampler.wrapU == TextureWrap::Repeat)           ? GL_REPEAT
                  : (asset.sampler.wrapU == TextureWrap::MirroredRepeat) ? GL_MIRRORED_REPEAT
                                                                         : GL_CLAMP_TO_EDGE;
    GLint wrapV = (asset.sampler.wrapV == TextureWrap::Repeat)           ? GL_REPEAT
                  : (asset.sampler.wrapV == TextureWrap::MirroredRepeat) ? GL_MIRRORED_REPEAT
                                                                         : GL_CLAMP_TO_EDGE;
    const GLint magFilter = asset.sampler.magFilter == TextureFilter::Nearest
                                  ? GL_NEAREST : GL_LINEAR;
    const GLint minFilter = !asset.sampler.generateMipmaps
                                  ? (asset.sampler.minFilter == TextureFilter::Nearest
                                         ? GL_NEAREST : GL_LINEAR)
                                  : (asset.sampler.minFilter == TextureFilter::Nearest
                                         ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    GLenum format = asset.format == TextureFormat::R8
                        ? GL_RED
                        : (asset.format == TextureFormat::RG8
                               ? GL_RG
                               : (asset.format == TextureFormat::RGB8 ? GL_RGB : GL_RGBA));
    GLint internalFormat = static_cast<GLint>(format);
    if (asset.colorSpace == TextureColorSpace::SRGB) {
        if (asset.format == TextureFormat::RGB8) {
            internalFormat = GL_SRGB8;
        } else if (asset.format == TextureFormat::RGBA8) {
            internalFormat = GL_SRGB8_ALPHA8;
        }
    }
    // 紧密对齐可正确上传任意通道数和行宽，完成后恢复调用方状态。
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, asset.width, asset.height,
                 0, format, GL_UNSIGNED_BYTE,
                 asset.pixels.data());
    if (asset.sampler.generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    return true;
}

void OpenGLTexture::bind(std::uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_);
}

void OpenGLTexture::shutdown() noexcept {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
}

} // namespace renderlab
