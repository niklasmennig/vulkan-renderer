#ifndef INC_INTERFACE_P
#define INC_INTERFACE_P

#define DESCRIPTOR_SET_BUFFERS 0
#define DESCRIPTOR_SET_IMAGES 1

struct PushConstants
{
    ivec2 swapchain_extent;
    ivec2 render_extent;
};


#endif