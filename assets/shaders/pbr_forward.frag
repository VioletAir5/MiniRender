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
            vec3 fresnel = fresnelSchlick(
                max(dot(halfway, viewDirection), 0.0), f0);
            float distribution = distributionGGX(normal, halfway, roughness);
            float geometry = geometrySmith(
                normal, viewDirection, lightDirection, roughness);
            vec3 specular = distribution * geometry * fresnel /
                max(4.0 * max(dot(normal, viewDirection), 0.0) *
                          max(dot(normal, lightDirection), 0.0), 0.0001);
            vec3 diffuse = (vec3(1.0) - fresnel) * (1.0 - metallic) *
                           color.rgb / PI;
            vec3 radiance = uLightColor * uLightIntensity;
            directLight = (diffuse + specular) * radiance *
                          max(dot(normal, lightDirection), 0.0);
        }
        litColor = color.rgb * 0.03 * (1.0 - metallic) +
                   directLight + uEmissive;
    }
    vec3 mapped = pow(litColor / (litColor + vec3(1.0)),
                      vec3(1.0 / 2.2));
    fragColor = vec4(mapped, color.a);
}
