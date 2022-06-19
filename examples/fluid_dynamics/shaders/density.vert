#version 450
#define PI 3.14159265358979

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(set = 1, binding = 2) buffer DENSITY {
    float densities[];
};

layout(push_constant) uniform SimData {
    int N;
    float maxMagintue;
};


layout(location = 0) in vec2 position;

layout(location = 0) out vec4 color;

void main(){
    int i = gl_InstanceIndex % N;
    int j = gl_InstanceIndex / N;
    int id = i * N + j;
    float density =  densities[id];
    color = vec4(1);
    color.a = density;

    float cellSize = 1/float(N);
    float x = i * cellSize;
    float y = j * cellSize;
    vec2 p = cellSize * position + vec2(x, y);
    gl_Position = projection * view * model  * vec4(p, 0, 1);
}