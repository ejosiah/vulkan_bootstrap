#version 460

layout(set = 1, binding = 0) uniform sampler2D diffuseMap;
layout(set = 1, binding = 1) uniform sampler2D ambientMap;
layout(set = 1, binding = 2) uniform sampler2D specularMap;

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 lightPos;
    vec2 uv;
} v_in;

const vec3 gAmb = vec3(0.5);

layout(location = 0) out vec4 fragColor;

float saturate(float value){
    return clamp(value, 0, 1);
}

const float levels = 2;
const float scaleFactor = 1/levels;

void main(){

//    vec3 L = normalize(v_in.lightPos - v_in.position);
    vec3 L = normalize(vec3(1, 1, 1));
    vec3 E = L;
    vec3 N = normalize(v_in.normal);
    vec3 H = normalize(E + L);

    vec3 albedo = texture(diffuseMap, v_in.uv).rgb;
    vec3 ambient = texture(ambientMap, v_in.uv).rgb;
    vec3 specular = texture(specularMap, v_in.uv).rgb;
    float LdotN = saturate(dot(L, N));
    vec3 color = gAmb * ambient + floor(LdotN * levels) * scaleFactor * albedo;
    fragColor = vec4(color, 1);
}