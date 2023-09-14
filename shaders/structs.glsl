#ifndef INC_STRUCTS
#define INC_STRUCTS

struct Light {
    uint uint_data[4];
    float float_data[16];
};

struct PushConstants {
    uint sbt_stride;
    float time;
    uint clear_accumulated;
    uint light_count;
    uint max_depth;
    uint flags;
    float padding1;
    float padding2;
};

#endif