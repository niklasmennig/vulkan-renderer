#ifndef PUSH_CONSTANTS_GLSL
#define PUSH_CONSTANTS_GLSL

#include "structs.glsl"
layout(std430, push_constant) uniform PConstants {PushConstantsPacked packed;} push_constants;

PushConstants get_push_constants() {
    PushConstantsPacked packed = push_constants.packed;
    PushConstants res;

    res.sbt_stride =        uint(packed.stride_lcount_fsample_depth & 0xFF000000) >> 24;
    res.light_count =       uint(packed.stride_lcount_fsample_depth & 0x00FF0000) >> 16;
    res.frame_samples =     uint(packed.stride_lcount_fsample_depth & 0x0000FF00) >> 8;
    res.max_depth =         uint(packed.stride_lcount_fsample_depth & 0x000000FF) >> 0;

    res.sample_count =  uint(packed.sample_count);
    res.frame =         uint(packed.frame);
    res.flags =         uint(packed.flags);

    res.environment_cdf_dimensions.x = uint(packed.env_dim_xy & 0xFFFF0000) >> 16;
    res.environment_cdf_dimensions.y = uint(packed.env_dim_xy & 0x0000FFFF) >> 0;

    res.swapchain_extent.x = uint(packed.sc_ext_xy & 0xFFFF0000) >> 16;
    res.swapchain_extent.y = uint(packed.sc_ext_xy & 0x0000FFFF) >> 0;

    res.render_extent.x = uint(packed.r_ext_xy & 0xFFFF0000) >> 16;
    res.render_extent.y = uint(packed.r_ext_xy & 0x0000FFFF) >> 0;

    res.exposure = packed.exposure;

    res.inv_camera_matrix = packed.inv_camera_matrix;
    res.camera_position = packed.camera_position;

    return res;
}

#endif