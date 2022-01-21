#version 460

layout(set = 0, binding = 0) uniform sampler2D intensityMap;

layout(push_constant) uniform CONSTANTS{
    int horizontal;
};


layout(location = 0) in vec2 frag_uv;

float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

layout(location = 0) out vec4 horizontalBlur;
layout(location = 1) out vec4 verticalBlur;

void main(){
    vec2 texOffset = 1/textureSize(intensityMap, 0);
    vec3 result = texture(intensityMap, frag_uv).rgb * weights[0];

    if(bool(horizontal)){
        for(int i = 0; i < 5; i++){
            result += texture(intensityMap, frag_uv + vec2(texOffset.x * i, 0)).rgb * weights[i];
            result += texture(intensityMap, frag_uv - vec2(texOffset.x * i, 0)).rgb * weights[i];
        }
        horizontalBlur = vec4(result, 1);
    }else{
        for(int i = 0; i < 5; i++){
            result += texture(intensityMap, frag_uv + vec2(0, texOffset.y * i)).rgb * weights[i];
            result += texture(intensityMap, frag_uv - vec2(0, texOffset.y * i)).rgb * weights[i];
        }
        verticalBlur = vec4(result, 1);
    }

}