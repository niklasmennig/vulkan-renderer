struct RayPayload
{
    vec3 color;
    vec3 contribution;

    uint max_depth;
    uint depth;
    uint seed;

    uint primary_hit_instance;
    vec3 primary_hit_albedo;
};