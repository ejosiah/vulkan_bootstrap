#version 450
#define TWO_PI 6.283185307179586476925286766559
#define NUM_SECTORS 32
#define MAX_VERTS (NUM_SECTORS + 1) * 3

layout(points) in;

layout(triangle_strip, max_vertices = MAX_VERTS) out;

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

layout(set = 2, binding = 0) buffer PARTICLE{
    vec2 particles[];
};


layout(push_constant) uniform SimData {
    int N;
    float maxMagintue;
    float dt;
};

layout(location = 0) in vec2 particleId[];

layout(location = 0) flat out vec2 out_velocity[];

void main(void)
{
    mat4 mvp = projection * view * model;

    float cellSize = 1/float(N);
    vec2 particle = particles[int(particleId[0].x)];
    vec2 position = particle;
    vec2 cellId = floor(position * N) + vec2(1,1);
    int index = int(cellId.y * (N+2) + cellId.x);

//    if(index >= 0 && index < N * N){
        vec2 v =  vec2(u[index], v[index]);
        position += v * dt;
        particles[int(particleId[0].x)] = position;
//    }

    float radius = 0.1 * cellSize;
    for(int i = 0; i <= NUM_SECTORS + 1; i++){
        gl_Position = mvp * vec4(position, 0, 1);
        out_velocity[0] = v;
        EmitVertex();

        float angle = float(i)/NUM_SECTORS  * TWO_PI;
        float x = radius * cos(angle) + position.x;
        float y = radius * sin(angle) + position.y;
        out_velocity[0] = v;

        gl_Position = mvp * vec4(x, y, 0, 1);
        EmitVertex();

        angle +=  TWO_PI/NUM_SECTORS;
        x = radius * cos(angle) + position.x;
        y = radius * sin(angle) + position.y;
        out_velocity[0] = v;
        gl_Position = mvp * vec4(x, y, 0, 1);
        EmitVertex();
    }
    EndPrimitive();
}