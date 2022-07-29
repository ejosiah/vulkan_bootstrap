#version 450

layout(set = 0, binding = 0) uniform sampler2D sourceField;
layout(set = 1, binding = 0) uniform sampler2D destinationField;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 value;

layout(push_constant) uniform Constants{
    float dt;
};

void main(){
    value = texture(sourceField, uv) * dt + texture(destinationField, uv);
}