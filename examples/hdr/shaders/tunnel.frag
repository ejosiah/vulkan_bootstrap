#version 460

struct Light{
    vec3 position;
    vec3 color;
};

layout(set = 0, binding = 0) buffer LIGHTS{
    Light lights[4];
};

layout(push_constant) uniform SETTINGS{
    layout(offset = 192)
    int gammaOn;
    int hdrOn;
    float exposure;
};

layout(set = 0, binding = 1) uniform sampler2D albedoMap;

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 eyes;
    vec2 uv;
    vec3 lightsPos[4];
} v_in;

layout(location = 0) out vec4 fragColor;

const float globalAmb = 0.3;

void main(){

    vec3 N = normalize(v_in.normal);
    N = gl_FrontFacing ? N : -N;
    vec3 E = normalize(v_in.eyes);

    vec3 radiance = vec3(0);
    for(int i = 0; i < 4; i++){
        vec3 lightColor = lights[i].color;
        vec3 lightDir = v_in.lightsPos[i] - v_in.position;
        vec3 L = normalize(lightDir);
        vec3 H = normalize(E + L);

        vec3 albedo = texture(albedoMap, v_in.uv).rgb;
        float diffuse = max(0, dot(N, L));
//        float specular = max(0, pow(dot(N, H), 50));
         vec3 localRadiance = lightColor * albedo * diffuse;
         float distance = length(lightDir);
        localRadiance *= 1/(distance * distance);
        radiance += localRadiance;
    }

    if(bool(hdrOn)){
        radiance = 1 - exp(-radiance * exposure);
    }

    float gamma =  bool(gammaOn) ? 2.2 : 1.0;
    radiance = pow(radiance, vec3(1/gamma));
    fragColor = vec4(radiance, 1);
}

