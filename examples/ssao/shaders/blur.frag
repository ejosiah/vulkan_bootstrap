#version 460

layout(set = 0, binding = 0) uniform sampler2D occlusion;

layout(location = 0) in vec2 uv;
layout(location = 0) out float fragColor;

layout(push_constant) uniform Constants{
    int blurOn;
};

void main(){
    if(!bool(blurOn)){
        fragColor = texture(occlusion, uv).r;
        return;
    }

    vec2 texelSize = 1.0/vec2(textureSize(occlusion, 0));
    float result = 0;
    for(int x = -2; x < 2; x++){
        for(int y = -2; y < 2; y++){
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(occlusion, uv + offset).r;
        }
    }
    result /= 16;
    fragColor = result;
}

