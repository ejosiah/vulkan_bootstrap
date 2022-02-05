#version 460 core

layout(set = 0, binding = 0) uniform sampler2D positionMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D colorMap;
layout(set = 0, binding = 3) uniform sampler2D occlusionMap;

layout(push_constant) uniform Constants {
    mat4 view;
    int aoOnly;
    int aoOn;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;


void main(){
    float ambientOcclusion = texture(occlusionMap, uv).r;
    if(bool(aoOn) && bool(aoOnly)){
        fragColor = vec4(ambientOcclusion);
        return;
    }

    ambientOcclusion = bool(aoOn) ? ambientOcclusion : 1.0;

    vec3 N = texture(normalMap, uv).xyz;
    N = normalize(N);

    vec3 lightDir = (view * vec4(1)).xyz;
    vec3 L = normalize(lightDir);



    vec3 color = texture(colorMap, uv).rgb;

    vec3 ambient = vec3(0.3) * color * ambientOcclusion;

    vec3 irradiance = max(0, dot(N, L)) * color;

    lightDir = (view * -vec4(1)).xyz;
    L = normalize(lightDir);
    irradiance += max(0, dot(N, L)) * color;

//    lightDir = (view * vec4(0, 1, 0, 1)).xyz;
//    L = normalize(lightDir);
//    irradiance += max(0, dot(N, L)) * color;

    irradiance += ambient;

    fragColor = vec4(irradiance, 1);
}