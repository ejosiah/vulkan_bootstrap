#version 450

//layout(points) in;					// size 1
//layout(lines) in;					// size 2
layout(triangles) in;					// size 3
//layout(line_adjacency) in;				// size 4
//layout(triangles_adjacency) in;				// size 6


layout(triangle_strip, max_vertices = 3) out;

void main(void)
{
    for (int n = 0; n < gl_in.length(); n++){
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}