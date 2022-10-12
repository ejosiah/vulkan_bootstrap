#define HIT_MARGIN 0.0001

struct RaytraceData{
    vec3 hitValue;
    int depth;
};

struct LightSource{
    vec3 locaiton;
    vec3 intensity;
};

struct Material{
    vec3 albedo;
    float metalness;
    float roughness;
    float pad0;
    float pad1;
};

struct GlassMaterial{
    vec3 albedo;
    float ior;
};

struct PatternParams{
    vec3 worldPos;
    vec3 normal;
    vec3 color;
    float scale;
};