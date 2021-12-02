#version 460

layout(set = 0, binding = 0) uniform sampler2D shadowMap;

layout(location = 0) in struct {
    vec4 lightSpacePos;
    vec3 position;
    vec3 normal;
    vec3 color;
    vec3 lightDir;
    vec3 eyePos;
} v_in;

layout(location = 0) out vec4 fragColor;

const vec3 globalAmb = vec3(0.3);

float shadowCalculation(vec4 lightSpacePos){
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    return shadow;
}

void main(){
    vec3 N = normalize(v_in.normal);
    vec3 L = normalize(v_in.lightDir);
    vec3 E = normalize(v_in.eyePos - v_in.position);
    vec3 H = normalize(L + E);

    vec3 amb = globalAmb * v_in.color;
    vec3 diffuse = max(0, dot(N, L)) * v_in.color;
    vec3 specular = max(0, pow(dot(N, H), 25)) * v_in.color;
    float shadow = shadowCalculation(v_in.lightSpacePos);
    vec3 color = amb + (1 - shadow) * (diffuse + specular);
    fragColor = vec4(color, 1);
}