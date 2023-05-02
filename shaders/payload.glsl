struct RayPayload
{
    vec3 next_origin;
    vec3 next_direction;
    vec3 direct_light;
    bool shadow_miss;
    float next_reflection_factor;
};