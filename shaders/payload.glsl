struct RayPayload
{
    uint depth;
    vec3 contribution;
    float seed;
};

struct MaterialPayload
{
    // provided when calling
    vec2 uv;
    vec3 position;
    vec3 normal;
    
    // returned from material shader
    vec3 emission;
};