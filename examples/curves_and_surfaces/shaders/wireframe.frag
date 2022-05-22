#version 450

layout(push_constant) uniform WIRE_FRAME{
layout(offset = 32)
    vec3 color;
    float width;
    int enabled;
    int solid;
} wireframe;

layout(location = 0) in struct {
    vec3 normal;
    vec3 worldPos;
    vec3 lightPos;
    vec3 eyePos;
} v_in;

layout(location = 4)  noperspective in vec3 edgeDist;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 albedo = vec3(1, 0, 0);
    vec3 N = normalize(v_in.normal);
    N = gl_FrontFacing ? N : -N;
    vec3 L = normalize(v_in.lightPos - v_in.worldPos);
    vec3 E = normalize(v_in.eyePos - v_in.worldPos);
    vec3 H = normalize(E + L);

    vec3 color = albedo * max(0, dot(N, L));


    if(bool(wireframe.enabled)){
        float d = min(edgeDist.x, min(edgeDist.y, edgeDist.z));
        float t = smoothstep(-wireframe.width, wireframe.width, d);

        if(!bool(wireframe.solid) && t > 0.7){
            discard;
        }else if(!bool(wireframe.solid) && t < 0.7){
            color = wireframe.color;
        }

        color = mix(wireframe.color, color, t);
    }else{
        color += vec3(1) * pow(max(0, dot(N, H)), 250);
    }

    fragColor = vec4(color, 1);
}