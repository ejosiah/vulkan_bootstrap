#version 450
#define PI 3.14159265358979

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(set = 1, binding = 0) buffer vector_u {
    float u[];
};

layout(set = 1, binding = 1) buffer vector_v {
    float v[];
};

layout(push_constant) uniform SimData {
    int N;
    float maxMagintue;
};

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 color;
layout(location = 1) flat out vec2 vector;

vec3 heatMap(vec3 u){
    float level = length(u)/maxMagintue;
    float freq = PI * 0.5;
    return vec3(
        sin(freq * level),
        sin(freq * 2 * level),
        cos(freq * level)
    );
}

void main(){
    int i = gl_InstanceIndex % (N+2);
    int j = gl_InstanceIndex / (N+2);
    int id = j * (N + 2) + i;
    vec3 v =  vec3(u[id], v[id], 0);
    vec3 v1 = normalize(v);
    float angle = 0;

    vec2 quadtrant = sign(v1).xy;

    bool q1 = all(equal(quadtrant, vec2(1, 1))) || all(equal(quadtrant, vec2(0, 1)));
    bool q2 = all(equal(quadtrant, vec2(-1, 1))) || all(equal(quadtrant, vec2(-1, 0)));
    bool q3 = all(equal(quadtrant, vec2(-1, -1))) || all(equal(quadtrant, vec2(0, -1)));
    bool q4 = all(equal(quadtrant, vec2(1, -1)));

    if(q1){
        angle = acos(dot(vec3(1, 0, 0), v1));
    }else if(q2){
        angle = acos(dot(vec3(0, 1, 0), v1)) + PI * 0.5;
    }else if(q3){
        angle = acos(dot(vec3(-1, 0, 0), v1)) + PI;
    }else if(q4){
        angle = acos(dot(vec3(0, -1, 0), v1)) + PI * 1.5;
    }

    mat3 rotor = mat3(
         cos(angle), sin(angle), 0,
        -sin(angle), cos(angle), 0,
                  0,          0, 1
    );

    float cellSize = 1/float(N+2);
    float x = (i) * cellSize;
    float y = (j) * cellSize;
    float scale = 0.3;
    vec2 p = vec2(rotor * (scale * position));

    p = p.xy + 0.5;   // [-.5, .5] -> [0, 1]
    p = cellSize * p + vec2(x, y);

    color = heatMap(v);
    vector = v.xy;
    gl_Position = projection * view * model  * vec4(p, 0, 1);
}