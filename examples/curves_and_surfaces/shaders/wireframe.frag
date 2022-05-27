#version 450

layout(set = 1, binding = 0) uniform Constants{
    vec3 obj_color;
    vec3 wireframe_color;
    float wireframe_width;
    int wireframe_enabled;
    int solid;
    int uvColor;
};

layout(set = 2, binding = 0) uniform sampler2D image;

layout(location = 0) in struct {
    vec3 normal;
    vec3 worldPos;
    vec3 lightPos;
    vec3 eyePos;
    vec2 uv;
} v_in;

layout(location = 5)  noperspective in vec3 edgeDist;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 N = normalize(v_in.normal);
    N = gl_FrontFacing ? N : -N;
    vec3 L = normalize(v_in.lightPos - v_in.worldPos);
    vec3 E = normalize(v_in.eyePos - v_in.worldPos);
    vec3 H = normalize(E + L);

//    vec4 imageColor = texture(image, v_in.uv).rgba;
    vec3 albedo = obj_color;
    vec3 color = bool(uvColor) ? albedo : albedo * max(0, dot(N, L));


    if(bool(wireframe_enabled)){
        float d = min(edgeDist.x, min(edgeDist.y, edgeDist.z));
        float t = smoothstep(-wireframe_width, wireframe_width, d);

        if(!bool(solid) && t > 0.7){
            discard;
        }else if(!bool(solid) && t < 0.7){
            color = wireframe_color;
        }

        color = mix(wireframe_color, color, t);
    }else if(!bool(uvColor)){
        color += vec3(1) * pow(max(0, dot(N, H)), 250);
    }

    fragColor = vec4(color, 1);
}