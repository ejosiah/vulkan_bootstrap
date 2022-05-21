#version 450

layout(quads, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(push_constant) uniform TESS_LEVELS{
    vec3 scale;
    float outer;
    float inner;
    float max_u;
    float max_v;
    int shouldNormalize;
};

layout(location = 0) out struct {
    vec3 normal;
    vec3 worldPos;
    vec3 lightPos;
    vec3 eyePos;
} v_out;

void main(){
    float u = min(gl_TessCoord.x, max_u);
    float v = min(gl_TessCoord.y, max_v);

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    vec3 p = mix(mix(p0, p1, u), mix(p3, p2, u), v);
    vec3 n;

    if(bool(shouldNormalize)){
        n = normalize(p).xyz;
        float r = max(max(scale.x, scale.y), scale.z);
        p = n * r;
    }else{
        n = cross(p1.xyz - p0.xyz, p2.xyz - p0.xyz);
        p *= scale;
    }

    vec4 position = model * vec4(p, 1);

    v_out.worldPos = position.xyz;
    v_out.normal = normalize(n);
    v_out.lightPos = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    v_out.eyePos = v_out.lightPos;

    gl_Position = projection * view * position;

}