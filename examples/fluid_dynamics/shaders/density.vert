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
    int i = gl_InstanceIndex % N + 1;
    int j = gl_InstanceIndex / N + 1;
    int id = j * (N + 2) + i;
    float density =  densities[id];
    color = vec4(1);
    color.a = density;

    float cellSize = 1/float(N);
    float x = (i-1) * cellSize;
    float y = (j-1) * cellSize;
    vec2 p = cellSize * position + vec2(x, y);
    gl_Position = projection * view * model  * vec4(p, 0, 1);
}