struct CameraData
{
    vec4 origin;
    vec4 forward;
    vec4 right;
    vec4 up;
    float fov_x;

    float time;
    uint clear_accumulated;
};
