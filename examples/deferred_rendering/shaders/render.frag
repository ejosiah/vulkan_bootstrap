#version 460

#define DISPLAY_LIGHTING 0
#define DISPLAY_ALBEDO 1
#define DISPLAY_NORMAL 2
#define DISPLAY_POSITION 3

layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput albedoAttachment;
layout(input_attachment_index=1, set=0, binding=1) uniform subpassInput normalAttachment;
layout(input_attachment_index=2, set=0, binding=2) uniform subpassInput positionAttachment;

layout(push_constant) uniform Constants{
    vec4 lightPos;
    uint option;
};

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 albedo = subpassLoad(albedoAttachment).rgb;
    vec3 normal = subpassLoad(normalAttachment).xyz;
    vec3 position = subpassLoad(positionAttachment).xyz;

    fragColor.a = 1;
    if (option == DISPLAY_ALBEDO){
        fragColor.rgb = albedo;
    } else if (option == DISPLAY_NORMAL){
        fragColor.rgb = normal;
    } else if (option == DISPLAY_POSITION){
        fragColor.rgb = position;
    } else if (option == DISPLAY_LIGHTING){
        vec3 N = normalize(normal);
        vec3 L = normalize(lightPos.xyz - position);
        vec3 color = max(0, dot(L, N)) * albedo;
        fragColor.rgb = color;
    }
    else{
        fragColor = vec4(0);
    }
}