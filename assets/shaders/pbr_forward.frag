#version 330 core

#define MAX_LIGHTS 8

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
uniform int uLightCount;
uniform int uLightTypes[MAX_LIGHTS];
uniform vec3 uLightPositions[MAX_LIGHTS];
uniform vec3 uLightDirections[MAX_LIGHTS];
uniform vec3 uLightColors[MAX_LIGHTS];
uniform float uLightIntensities[MAX_LIGHTS];
uniform float uLightRanges[MAX_LIGHTS];
uniform float uLightInnerConeCosines[MAX_LIGHTS];
uniform float uLightOuterConeCosines[MAX_LIGHTS];

uniform sampler2D uDirectionalShadowMap;
uniform int uShadowLightIndex;
uniform int uShadowTechnique;
uniform float uShadowBias;

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec4 vWorldTangent;
in vec2 vTexCoord;
in vec4 vLightSpacePosition;

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

float directionalShadow(vec3 normal, vec3 lightDirection)
{
    if (uShadowLightIndex < 0 || uShadowTechnique == 0) {
        return 0.0;
    }

    vec3 projected = vLightSpacePosition.xyz /
                     max(vLightSpacePosition.w, 0.00001);
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0 ||
        projected.x <= 0.0 || projected.x >= 1.0 ||
        projected.y <= 0.0 || projected.y >= 1.0) {
        return 0.0;
    }

    float bias = max(uShadowBias * (1.0 - dot(normal, lightDirection)),
                     uShadowBias * 0.25);
    float receiverDepth = projected.z - bias;
    if (uShadowTechnique == 1) {
        return receiverDepth >
               texture(uDirectionalShadowMap, projected.xy).r ? 1.0 : 0.0;
    }

    vec2 texel = 1.0 / vec2(textureSize(uDirectionalShadowMap, 0));
    float blocked = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float closest = texture(
                uDirectionalShadowMap,
                projected.xy + vec2(x, y) * texel).r;
            blocked += receiverDepth > closest ? 1.0 : 0.0;
        }
    }
    return blocked / 9.0;
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
        vec4 sampleValue = texture(
            uMetallicRoughnessTexture,
            transformUv(vTexCoord, uMetallicRoughnessUvScale,
                        uMetallicRoughnessUvOffset,
                        uMetallicRoughnessUvRotation));
        roughness *= sampleValue.g;
        metallic *= sampleValue.b;
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
        for (int i = 0; i < uLightCount; ++i) {
            vec3 lightDirection;
            float attenuation = 1.0;
            if (uLightTypes[i] == 0) {
                lightDirection = normalize(-uLightDirections[i]);
            } else {
                vec3 toLight = uLightPositions[i] - vWorldPosition;
                float distanceToLight = length(toLight);
                lightDirection = toLight / max(distanceToLight, 0.0001);
                float rangeFade = clamp(
                    1.0 - distanceToLight / uLightRanges[i], 0.0, 1.0);
                attenuation = rangeFade * rangeFade /
                    max(1.0 + distanceToLight * distanceToLight * 0.05,
                        0.0001);
                if (uLightTypes[i] == 2) {
                    vec3 lightToSurface = -lightDirection;
                    float coneCosine = dot(
                        normalize(uLightDirections[i]), lightToSurface);
                    attenuation *= smoothstep(
                        uLightOuterConeCosines[i],
                        uLightInnerConeCosines[i], coneCosine);
                }
            }

            vec3 halfway = normalize(viewDirection + lightDirection);
            vec3 f0 = mix(vec3(0.04), color.rgb, metallic);
            vec3 fresnel = fresnelSchlick(
                max(dot(halfway, viewDirection), 0.0), f0);
            float distribution =
                distributionGGX(normal, halfway, roughness);
            float geometry = geometrySmith(
                normal, viewDirection, lightDirection, roughness);
            vec3 specular = distribution * geometry * fresnel /
                max(4.0 * max(dot(normal, viewDirection), 0.0) *
                          max(dot(normal, lightDirection), 0.0), 0.0001);
            vec3 diffuse = (vec3(1.0) - fresnel) *
                           (1.0 - metallic) * color.rgb / PI;
            float visibility = i == uShadowLightIndex
                ? 1.0 - directionalShadow(normal, lightDirection)
                : 1.0;
            vec3 radiance = uLightColors[i] * uLightIntensities[i] *
                            attenuation * visibility;
            directLight += (diffuse + specular) * radiance *
                           max(dot(normal, lightDirection), 0.0);
        }
        litColor = color.rgb * 0.03 * (1.0 - metallic) +
                   directLight + uEmissive;
    }

    vec3 mapped = pow(litColor / (litColor + vec3(1.0)),
                      vec3(1.0 / 2.2));
    fragColor = vec4(mapped, color.a);
}
