#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <assets/TextureAsset.h>



namespace renderlab {
class OpenGLTexture final {
  public:
    OpenGLTexture() = default;
    ~OpenGLTexture();

    OpenGLTexture(const OpenGLTexture&) = delete;
    OpenGLTexture& operator=(const OpenGLTexture&) = delete;

    OpenGLTexture(OpenGLTexture&& other) noexcept;
    OpenGLTexture& operator=(OpenGLTexture&& other) noexcept;

    bool upload(const TextureAsset& asset);
    void bind(std::uint32_t unit) const;
    void shutdown() noexcept;

  private:
    GLuint texture_{0};
};
} // namespace renderlab
