#include "renderer/backends/opengl/OpenGLShaderProgram.h"

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

namespace renderlab {
namespace {

// 基础 PBR Shader：Cook-Torrance 直接光照、材质 factor 与 Base Color 纹理。
constexpr std::string_view VertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 4) in vec4 aTangent;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;


out vec2 vTexCoord;
out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec4 vWorldTangent;

void main()
{
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize(transpose(inverse(mat3(uModel))) * aNormal);
    vec3 worldTangent = normalize(mat3(uModel) * aTangent.xyz);
    float orientation = determinant(mat3(uModel)) < 0.0 ? -1.0 : 1.0;
    vWorldTangent = vec4(worldTangent, aTangent.w * orientation);
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * worldPosition;
}
)";

constexpr std::string_view FragmentShaderSource = R"(
#version 330 core

uniform vec4 uBaseColor;
uniform sampler2D uBaseColorTexture;
uniform bool uHasBaseColorTexture;

uniform vec2 uUvOffset;
uniform vec2 uUvScale;
uniform float uUvRotation;
uniform int uAlphaMode;
uniform float uAlphaCutoff;

uniform float uMetallic;
uniform float uRoughness;
uniform sampler2D uMetallicRoughnessTexture;
uniform bool uHasMetallicRoughnessTexture;
uniform vec2 uMetallicRoughnessUvOffset;
uniform vec2 uMetallicRoughnessUvScale;
uniform float uMetallicRoughnessUvRotation;
uniform sampler2D uNormalTexture;
uniform bool uHasNormalTexture;
uniform vec2 uNormalUvOffset;
uniform vec2 uNormalUvScale;
uniform float uNormalUvRotation;
uniform float uNormalScale;
uniform vec3 uEmissive;
uniform bool uUnlit;

uniform vec3 uCameraPosition;
uniform bool uHasDirectionalLight;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec4 vWorldTangent;
in vec2 vTexCoord;

out vec4 fragColor;

const float PI = 3.14159265359;

float distributionGGX(vec3 normal, vec3 halfway, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(normal, halfway), 0.0);
    float denominator = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denominator * denominator, 0.000001);
}

float geometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 0.000001);
}

float geometrySmith(vec3 normal, vec3 viewDirection,
                    vec3 lightDirection, float roughness)
{
    return geometrySchlickGGX(max(dot(normal, viewDirection), 0.0), roughness) *
           geometrySchlickGGX(max(dot(normal, lightDirection), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 reflectance)
{
    return reflectance + (1.0 - reflectance) * pow(1.0 - cosTheta, 5.0);
}

vec2 transformUv(vec2 uv, vec2 scale, vec2 offset, float rotation)
{
    mat2 rotationMatrix = mat2(cos(rotation), -sin(rotation),
                               sin(rotation),  cos(rotation));
    return rotationMatrix * (uv * scale) + offset;
}

vec3 resolveNormal()
{
    vec3 normal = normalize(vWorldNormal);
    if (uHasNormalTexture) {
        vec3 tangent = normalize(vWorldTangent.xyz -
                                 normal * dot(normal, vWorldTangent.xyz));
        vec3 bitangent = normalize(cross(normal, tangent)) * vWorldTangent.w;
        vec3 tangentNormal = texture(
            uNormalTexture,
            transformUv(vTexCoord, uNormalUvScale,
                        uNormalUvOffset, uNormalUvRotation)).xyz;
        tangentNormal = tangentNormal * 2.0 - 1.0;
        tangentNormal.xy *= uNormalScale;
        normal = normalize(mat3(tangent, bitangent, normal) *
                           normalize(tangentNormal));
    }
    // 双面材质的背面法线必须翻转，否则背面会使用正面的光照方向。
    return gl_FrontFacing ? normal : -normal;
}

void main()
{
    vec4 sampledColor = uHasBaseColorTexture

        ? texture(uBaseColorTexture,
                  transformUv(vTexCoord, uUvScale, uUvOffset, uUvRotation))
        : vec4(1.0);
    vec4 color = uBaseColor * sampledColor;
    if (uAlphaMode == 1 && color.a < uAlphaCutoff) discard;
    if (uAlphaMode == 0) color.a = 1.0;

    float metallic = uMetallic;
    float roughness = uRoughness;
    if (uHasMetallicRoughnessTexture) {
        vec4 metallicRoughnessSample = texture(
            uMetallicRoughnessTexture,
            transformUv(vTexCoord, uMetallicRoughnessUvScale,
                        uMetallicRoughnessUvOffset,
                        uMetallicRoughnessUvRotation));
        // glTF 2.0 规定 G 通道为 roughness，B 通道为 metallic。
        roughness *= metallicRoughnessSample.g;
        metallic *= metallicRoughnessSample.b;
    }
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 litColor;
    if (uUnlit) {
        litColor = color.rgb + uEmissive;
    } else {
        vec3 normal = resolveNormal();
        vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
        vec3 directLight = vec3(0.0);
        if (uHasDirectionalLight) {
            vec3 lightDirection = normalize(-uLightDirection);
            vec3 halfway = normalize(viewDirection + lightDirection);
            vec3 f0 = mix(vec3(0.04), color.rgb, metallic);
            vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDirection), 0.0), f0);
            float distribution = distributionGGX(normal, halfway, roughness);
            float geometry = geometrySmith(normal, viewDirection, lightDirection, roughness);
            vec3 specular = distribution * geometry * fresnel /
                max(4.0 * max(dot(normal, viewDirection), 0.0) *
                          max(dot(normal, lightDirection), 0.0), 0.0001);
            vec3 diffuse = (vec3(1.0) - fresnel) * (1.0 - metallic) * color.rgb / PI;
            vec3 radiance = uLightColor * uLightIntensity;
            directLight = (diffuse + specular) * radiance *
                          max(dot(normal, lightDirection), 0.0);
        }
        litColor = color.rgb * 0.03 * (1.0 - metallic) +
                   directLight + uEmissive;
    }
    vec3 mapped = pow(litColor / (litColor + vec3(1.0)), vec3(1.0 / 2.2));
    fragColor = vec4(mapped, color.a);
}
)";

// 将当前支持的着色器阶段转换为日志名称。
const char* shaderStageName(const GLenum stage) {
    return stage == GL_VERTEX_SHADER ? "vertex" : "fragment";
}

// 编译单阶段着色器并记录驱动日志；失败时返回零。
GLuint compileShader(const GLenum stage, const std::string_view source) {
    const GLuint shader = glCreateShader(stage);
    const GLchar* sourceData = source.data();
    const GLint sourceLength = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &sourceData, &sourceLength);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    spdlog::error("OpenGL {} shader compilation failed: {}", shaderStageName(stage), log);
    glDeleteShader(shader);
    return 0;
}

// 读取并输出程序链接日志。
void logProgramLinkError(const GLuint program) {
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    glGetProgramInfoLog(program, logLength, nullptr, log.data());
    spdlog::error("OpenGL program link failed: {}", log);
}

} // namespace

bool OpenGLShaderProgram::initialize() {
    shutdown();

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VertexShaderSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FragmentShaderSource);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
        }
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);

    // 链接后程序已持有编译结果，可立即释放独立 shader 对象。
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        logProgramLinkError(program_);
        shutdown();
        return false;
    }
    return true;
}

void OpenGLShaderProgram::shutdown() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void OpenGLShaderProgram::bind() const {
    glUseProgram(program_);
}

void OpenGLShaderProgram::release() {
    glUseProgram(0);
}

void OpenGLShaderProgram::setMatrix(const char* name, const glm::mat4& value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void OpenGLShaderProgram::setVector4(
    const char* name, const glm::vec4& value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void OpenGLShaderProgram::setInteger(const char* name, const int value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform1i(location, value);
    }
}

void OpenGLShaderProgram::setVector2(
    const char* name, const glm::vec2& value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) glUniform2fv(location, 1, glm::value_ptr(value));
}

void OpenGLShaderProgram::setFloat(const char* name, const float value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) glUniform1f(location, value);
}


void OpenGLShaderProgram::setVector3(
    const char* name, const glm::vec3& value) const {
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) glUniform3fv(location, 1, glm::value_ptr(value));
}
} // namespace renderlab
