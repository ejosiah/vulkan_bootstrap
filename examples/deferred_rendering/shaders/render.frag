#version 460

#define DISPLAY_LIGHTING 0
#define DISPLAY_ALBEDO 1
#define DISPLAY_NORMAL 2
#define DISPLAY_POSITION 3

layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput albedoAttachment;
layout(input_attachment_index=1, set=0, binding=1) uniform subpassInput normalAttachment;
layout(input_attachment_index=2, set=0, binding=2) uniform subpassInput positionAttachment;
layout(input_attachment_index=3, set=0, binding=3) uniform subpassInput emissionAttachment;

struct Light{
    vec4 position;
    vec4 color;
};

layout(set=1, binding=0) buffer Lights{
    Light lights[];
};

layout(push_constant) uniform Constants{
    uint numLights;
    uint option;
};

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 fragColor;

const float kc = 1;
const float kl = 1;
const float kq = 1;

void main(){

    fragColor.a = 1;
    if (option == DISPLAY_ALBEDO){
        fragColor.rgb = subpassLoad(albedoAttachment).rgb;
    } else if (option == DISPLAY_NORMAL){
        fragColor.rgb = subpassLoad(normalAttachment).xyz;
    } else if (option == DISPLAY_POSITION){
        fragColor.rgb = subpassLoad(positionAttachment).xyz;
    } else if (option == DISPLAY_LIGHTING){

        vec3 color = vec3(0);
        vec3 albedo = subpassLoad(albedoAttachment).rgb;
        vec3 normal = subpassLoad(normalAttachment).xyz;
        vec3 position = subpassLoad(positionAttachment).xyz;

        for(int i = 0; i < numLights; i++){
            Light light = lights[i];
            vec3 lightDir = light.position.xyz - position;
            float d = length(lightDir);
            vec3 N = normalize(normal);
            vec3 L = lightDir/d;

            color += (light.color.rgb * max(0, dot(L, N)) * albedo)/(kc + kl * d + kq * d * d);
        }

        vec3 emission = subpassLoad(emissionAttachment).rgb;
        color += emission;

        fragColor.rgb = pow(color, vec3(0.45));
    }
    else{
        fragColor = vec4(0);
    }
}