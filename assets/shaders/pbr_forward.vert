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
