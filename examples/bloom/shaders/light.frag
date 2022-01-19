#version 460

layout(location = 0) in vec3 lightColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragBrightness;

const vec3 brightnessThreshold = vec3(0.2126, 0.7152, 0.0722);

void main(){
    fragColor = vec4(lightColor, 1);

    if(dot(lightColor, brightnessThreshold) > 1){
        fragBrightness = fragColor;
    }else{
        fragBrightness = vec4(0, 0, 0, 1);
    }
}