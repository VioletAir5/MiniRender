#include "renderer/backends/opengl/OpenGLRenderResources.h"

#include <algorithm>
#include <utility>

namespace renderlab {
namespace {

struct OpenGLTextureFormat {
    GLint internalFormat{0};
    GLenum format{0};
    GLenum type{0};
    GLenum attachment{0};
};

std::optional<OpenGLTextureFormat> toOpenGLFormat(const RenderResourceFormat format) {
    switch (format) {
    case RenderResourceFormat::Rgba8Unorm:
        return OpenGLTextureFormat{GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0};
    case RenderResourceFormat::Rgba16Float:
        return OpenGLTextureFormat{GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0};
    case RenderResourceFormat::R32UnsignedInteger:
        return OpenGLTextureFormat{GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, GL_COLOR_ATTACHMENT0};
    case RenderResourceFormat::Depth24Stencil8:
        return OpenGLTextureFormat{GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                                   GL_DEPTH_STENCIL_ATTACHMENT};
    case RenderResourceFormat::Depth32Float:
        return OpenGLTextureFormat{GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT,
                                   GL_DEPTH_ATTACHMENT};
    case RenderResourceFormat::Unknown:
        break;
    }
    return std::nullopt;
}

std::string framebufferKey(const std::string_view color, const std::string_view depth) {
    return std::string{color} + "|" + std::string{depth};
}

} // namespace

bool OpenGLRenderResources::initialize(std::vector<RenderResourceDescriptor> descriptors,
                                       const int viewportWidth, const int viewportHeight,
                                       std::string& error) {
    shutdown();
    error.clear();
    viewportWidth_ = std::max(viewportWidth, 0);
    viewportHeight_ = std::max(viewportHeight, 0);
    resources_.reserve(descriptors.size());
    for (RenderResourceDescriptor& descriptor : descriptors) {
        resources_.push_back(Resource{.descriptor = std::move(descriptor)});
    }
    return recreateTransientResources(error);
}

void OpenGLRenderResources::shutdown() noexcept {
    destroyTransientResources();
    resources_.clear();
    externalFramebuffer_ = 0;
    viewportWidth_ = 0;
    viewportHeight_ = 0;
}

bool OpenGLRenderResources::resize(const int viewportWidth, const int viewportHeight,
                                   std::string& error) {
    const int normalizedWidth = std::max(viewportWidth, 0);
    const int normalizedHeight = std::max(viewportHeight, 0);
    if (normalizedWidth == viewportWidth_ && normalizedHeight == viewportHeight_) {
        error.clear();
        return true;
    }
    viewportWidth_ = normalizedWidth;
    viewportHeight_ = normalizedHeight;
    return recreateTransientResources(error);
}

void OpenGLRenderResources::setExternalFramebuffer(const GLuint framebuffer) noexcept {
    externalFramebuffer_ = framebuffer;
}

void OpenGLRenderResources::bindExternalFramebuffer() const {
    glBindFramebuffer(GL_FRAMEBUFFER, externalFramebuffer_);
    glViewport(0, 0, viewportWidth_, viewportHeight_);
}

GLuint OpenGLRenderResources::texture(const std::string_view name) const noexcept {
    const Resource* resource = find(name);
    return resource != nullptr ? resource->texture : 0;
}

std::optional<RenderResourceExtent>
OpenGLRenderResources::extent(const std::string_view name) const noexcept {
    const Resource* resource = find(name);
    return resource != nullptr ? resource->extent : std::nullopt;
}

bool OpenGLRenderResources::bindRenderTargets(const std::string_view color,
                                              const std::string_view depth, std::string& error) {
    error.clear();
    Resource* colorResource = color.empty() ? nullptr : find(color);
    Resource* depthResource = depth.empty() ? nullptr : find(depth);
    if ((!color.empty() && colorResource == nullptr) ||
        (!depth.empty() && depthResource == nullptr)) {
        error = "Unknown OpenGL render target";
        return false;
    }

    const bool colorExternal = colorResource != nullptr && colorResource->descriptor.external;
    const bool depthExternal = depthResource != nullptr && depthResource->descriptor.external;
    if (colorExternal || depthExternal) {
        if ((colorResource != nullptr && !colorExternal) ||
            (depthResource != nullptr && !depthExternal)) {
            error = "Cannot mix external and transient framebuffer attachments";
            return false;
        }
        bindExternalFramebuffer();
        return true;
    }
    if (colorResource == nullptr && depthResource == nullptr) {
        error = "At least one render target attachment is required";
        return false;
    }
    if ((colorResource != nullptr && colorResource->texture == 0) ||
        (depthResource != nullptr && depthResource->texture == 0)) {
        error = "Render target has no allocated texture";
        return false;
    }
    if (colorResource != nullptr &&
        toOpenGLFormat(colorResource->descriptor.format)->attachment != GL_COLOR_ATTACHMENT0) {
        error = "Color render target uses a depth format: " + colorResource->descriptor.name;
        return false;
    }
    if (depthResource != nullptr &&
        toOpenGLFormat(depthResource->descriptor.format)->attachment == GL_COLOR_ATTACHMENT0) {
        error = "Depth render target uses a color format: " + depthResource->descriptor.name;
        return false;
    }

    const auto targetExtent =
        colorResource != nullptr ? colorResource->extent : depthResource->extent;
    if (!targetExtent.has_value() || (colorResource != nullptr && depthResource != nullptr &&
                                      colorResource->extent != depthResource->extent)) {
        error = "Render target attachments have incompatible extents";
        return false;
    }

    const std::string key = framebufferKey(color, depth);
    GLuint framebuffer = 0;
    if (const auto existing = framebuffers_.find(key); existing != framebuffers_.end()) {
        framebuffer = existing->second;
    } else {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        if (colorResource != nullptr) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   colorResource->texture, 0);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
        } else {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }
        if (depthResource != nullptr) {
            const auto format = toOpenGLFormat(depthResource->descriptor.format);
            glFramebufferTexture2D(GL_FRAMEBUFFER, format->attachment, GL_TEXTURE_2D,
                                   depthResource->texture, 0);
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glDeleteFramebuffers(1, &framebuffer);
            bindExternalFramebuffer();
            error = "OpenGL framebuffer is incomplete";
            return false;
        }
        framebuffers_.emplace(key, framebuffer);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, targetExtent->width, targetExtent->height);
    return true;
}

bool OpenGLRenderResources::recreateTransientResources(std::string& error) {
    destroyTransientResources();
    error.clear();
    for (Resource& resource : resources_) {
        resource.extent =
            resolveRenderResourceExtent(resource.descriptor, viewportWidth_, viewportHeight_);
        if (resource.descriptor.external) {
            continue;
        }
        if (resource.descriptor.kind != RenderResourceKind::Texture2D) {
            error = "OpenGL transient buffers are not implemented: " + resource.descriptor.name;
            destroyTransientResources();
            return false;
        }
        if (!resource.extent.has_value()) {
            // 视口尚未建立时延迟到 resize。
            if (resource.descriptor.sizeMode == RenderResourceSizeMode::Viewport) {
                continue;
            }
            error = "Cannot resolve render resource extent: " + resource.descriptor.name;
            destroyTransientResources();
            return false;
        }
        if (!createTexture(resource, error)) {
            destroyTransientResources();
            return false;
        }
    }
    return true;
}

bool OpenGLRenderResources::createTexture(Resource& resource, std::string& error) {
    const auto format = toOpenGLFormat(resource.descriptor.format);
    if (!format.has_value()) {
        error = "Unsupported OpenGL render resource format: " + resource.descriptor.name;
        return false;
    }

    glGenTextures(1, &resource.texture);
    if (resource.texture == 0) {
        error = "OpenGL failed to create render texture: " + resource.descriptor.name;
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, resource.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format->internalFormat, resource.extent->width,
                 resource.extent->height, 0, format->format, format->type, nullptr);
    const bool nearest = resource.descriptor.format == RenderResourceFormat::R32UnsignedInteger ||
                         resource.descriptor.format == RenderResourceFormat::Depth24Stencil8 ||
                         resource.descriptor.format == RenderResourceFormat::Depth32Float;
    const GLint filter = nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return resource.texture != 0;
}

void OpenGLRenderResources::destroyTransientResources() noexcept {
    for (const auto& [key, framebuffer] : framebuffers_) {
        (void)key;
        if (framebuffer != 0) {
            glDeleteFramebuffers(1, &framebuffer);
        }
    }
    framebuffers_.clear();
    for (Resource& resource : resources_) {
        if (resource.texture != 0) {
            glDeleteTextures(1, &resource.texture);
            resource.texture = 0;
        }
        resource.extent.reset();
    }
}

OpenGLRenderResources::Resource* OpenGLRenderResources::find(const std::string_view name) noexcept {
    const auto iterator =
        std::find_if(resources_.begin(), resources_.end(),
                     [name](const Resource& resource) { return resource.descriptor.name == name; });
    return iterator != resources_.end() ? &*iterator : nullptr;
}

const OpenGLRenderResources::Resource*
OpenGLRenderResources::find(const std::string_view name) const noexcept {
    const auto iterator =
        std::find_if(resources_.begin(), resources_.end(),
                     [name](const Resource& resource) { return resource.descriptor.name == name; });
    return iterator != resources_.end() ? &*iterator : nullptr;
}

} // namespace renderlab
