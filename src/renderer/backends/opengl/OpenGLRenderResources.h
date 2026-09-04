#pragma once

#include "renderer/pipeline/RenderGraph.h"

#include <glad/glad.h>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace renderlab {

// OpenGL 对 RenderGraph 逻辑纹理的后端映射。GLuint 不会离开 OpenGL 目录。
class OpenGLRenderResources final {
  public:
    OpenGLRenderResources() = default;
    OpenGLRenderResources(const OpenGLRenderResources&) = delete;
    OpenGLRenderResources& operator=(const OpenGLRenderResources&) = delete;

    [[nodiscard]] bool initialize(std::vector<RenderResourceDescriptor> descriptors,
                                  int viewportWidth, int viewportHeight, std::string& error);
    void shutdown() noexcept;
    [[nodiscard]] bool resize(int viewportWidth, int viewportHeight, std::string& error);

    // 当前外部 FBO 每帧可能由 Qt 改变，因此由 Backend 在执行 Pipeline 前更新。
    void setExternalFramebuffer(GLuint framebuffer) noexcept;
    void bindExternalFramebuffer() const;

    [[nodiscard]] GLuint texture(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<RenderResourceExtent> extent(std::string_view name) const noexcept;

    // color/depth 可为空；具名资源必须全部为 transient 或全部为 external。
    [[nodiscard]] bool bindRenderTargets(std::string_view color, std::string_view depth,
                                         std::string& error);

  private:
    struct Resource {
        RenderResourceDescriptor descriptor;
        GLuint texture{0};
        std::optional<RenderResourceExtent> extent;
    };

    [[nodiscard]] bool recreateTransientResources(std::string& error);
    [[nodiscard]] bool createTexture(Resource& resource, std::string& error);
    void destroyTransientResources() noexcept;
    [[nodiscard]] Resource* find(std::string_view name) noexcept;
    [[nodiscard]] const Resource* find(std::string_view name) const noexcept;

    std::vector<Resource> resources_;
    std::unordered_map<std::string, GLuint> framebuffers_;
    GLuint externalFramebuffer_{0};
    int viewportWidth_{0};
    int viewportHeight_{0};
};

} // namespace renderlab
