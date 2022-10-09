struct Material{
    vec3 diffuse;
    vec3 ambient;
    vec3 specular;
    vec3 emission;
    vec3 transmittance;
    float shininess;
    float ior;
    float opacity;
    float illum;
};

struct VertexOffsets{
    int firstIndex;
    int vertexOffset;
    int material;
    int padding1;
};

struct Vertex{
    vec3 position;
    vec3 color;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
};

struct SceneObject{
    mat4 xform;
    mat4 xformIT;
    int objId;
    int padding0;
    int padding1;
    int padding2;
};