struct RayPayload
{
    uint depth;
    vec3 contribution;
    float seed;
};

struct MaterialPayload
{
    // provided when calling
    uint instance;
    vec2 uv;
    vec3 position;
    vec3 normal;
    float seed;
    
    // returned from material shader
    vec3 emission;
    vec3 surface_color;
    vec3 reflection_direction;
};