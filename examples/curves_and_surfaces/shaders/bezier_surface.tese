#version 450

layout(quads, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};


const vec4 bc = vec4(1, 3, 3, 1);
const float epsilon = 0.001;

float B(int i, float u){
    return bc[i] * pow(1 - u, 3 - i) * pow(u, i);
}

float dB(int i, float u){
    float n = 3;
    return
        bc[i] * (pow(u, i) * (n - i) * pow((1-u), n-i-1)
        + pow((1-u), n-1) * i * pow(u, i-1));
}

vec4 f(float u, float v, int i, int j){
    int ij = i * 4 + j;
    return B(i, u) * B(j, v) * gl_in[ij].gl_Position;
}

vec4 dfdu(float u, float v, int i, int j){
    int ij = i * 4 + j;
    return dB(i, u) * B(j, v) * gl_in[ij].gl_Position;
}

vec4 dfdv(float u, float v, int i, int j){
    int ij = i * 4 + j;
    return B(i, u) * dB(j, v) * gl_in[ij].gl_Position;
}

layout(location = 0) out struct {
    vec3 normal;
    vec3 worldPos;
    vec3 lightPos;
    vec3 eyePos;
} v_out;

void main(){
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 p = vec4(0);
    vec3 t = vec3(0);
    vec3 b = vec3(0);
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            int ij = i * 4 + j;
            vec4 fp = f(u, v, i, j);
//            vec3 tp = f(u + epsilon, v, i, j).xyz;
//            vec3 bp = f(u, v + epsilon, i, j).xyz;
//            t += (tp - fp.xyz)/epsilon;
//            b += (bp - fp.xyz)/epsilon;
            vec3 tp = -dfdu(u, v, i, j).xyz;
            vec3 bp = -dfdv(u, v, i, j).xyz;
            p += fp;
            t += tp;
            b += bp;
        }
    }
    vec4 position = model * p;
    v_out.worldPos = position.xyz;
    v_out.normal = normalize(cross(t, b));
    v_out.lightPos = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    v_out.eyePos = v_out.lightPos;

    gl_Position = projection * view * position;

}