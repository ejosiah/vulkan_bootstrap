#version 460

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POSITIONAL 1

layout(set = 1, binding = 0) uniform sampler2D shadowMap;

layout(push_constant) uniform CONSTANTS{
    int lightType;
};

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
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    if(projCoords.z > 1.0){
        return 0.0;
    }
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    return shadow;
}
float pcfFilteredShadow(vec4 lightSpacePos){
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    if(projCoords.z > 1.0){
        return 0.0;
    }
    float shadow = 0.0f;
    float currentDepth = projCoords.z;
    vec2 texelSize = 1.0/textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; x++){
        for(int y = -1; y <= 1; y++){
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow/9.0;
}

void main(){
    vec3 N = normalize(v_in.normal);
    vec3 L;

    if(lightType == LIGHT_TYPE_DIRECTIONAL){
        L = normalize(v_in.lightDir);
    }else{
        L = normalize(v_in.lightDir - v_in.position);
    }

    vec3 E = normalize(v_in.eyePos - v_in.position);
    vec3 H = normalize(L + E);

    vec3 amb = globalAmb * v_in.color;
    vec3 diffuse = max(0, dot(N, L)) * v_in.color;
    vec3 specular = max(0, pow(dot(N, H), 25)) * v_in.color;
    float shadow = pcfFilteredShadow(v_in.lightSpacePos);
    vec3 color = amb + (1 - shadow) * (diffuse);
    fragColor = vec4(color, 1);
}