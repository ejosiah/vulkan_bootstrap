#version 460

struct Light{
    vec3 position;
    vec3 color;
};

layout(set = 0, binding = 0) buffer LIGHTS{
    Light lights[4];
};

layout(set = 1, binding = 0) uniform sampler2D colorMap;

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec2 uv;
} frag_in;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragBrightness;

const float globalAmbiance = 0.0;
const vec3 brightnessThreshold = vec3(0.2126, 0.7152, 0.0722);

void main(){
    vec3 N = normalize(frag_in.normal);
    vec3 color = texture(colorMap, frag_in.uv).rgb;

    vec3 totalRadiance = vec3(0);
    for(int i = 0; i < 4; i++){
        vec3 lightDir = normalize(lights[i].position - frag_in.position);
        float distance = length(lightDir);
        float attenumation = 1/(distance * distance);

        vec3 L = normalize(lightDir);
        float diffuse = max(0, dot(L, N));
        vec3 lightColor = lights[i].color;
        vec3 localRadiance = lightColor * color * (globalAmbiance + diffuse);
        totalRadiance += localRadiance;
    }
    fragColor = vec4(totalRadiance, 1);

    if(dot(totalRadiance, brightnessThreshold) > 1){
        fragBrightness = fragColor;
    }else{
        fragBrightness = vec4(0, 0, 0, 1);
    }

}
