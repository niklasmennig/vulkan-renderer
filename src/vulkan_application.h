#include "vulkan.h"
#include "device.h"
#include "buffer.h"
#include "pipeline_builder.h"

#include <vector>
#include <optional>

#include <glm/glm.hpp>
using vec3 = glm::vec3;
using vec4 = glm::vec4;
namespace Shaders
{
    #include "../shaders/camera.glsl"
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
};

struct MeshData {
    VkDevice device_handle;
    uint32_t vertex_count;
    uint32_t normal_count;
    uint32_t vertex_index_count;
    uint32_t normal_index_count;
    Buffer vertices;
    Buffer vertex_indices;
    Buffer normals;
    Buffer normal_indices;

    void free();
};

struct AccelerationStructure {
    VkAccelerationStructureKHR acceleration_structure;
    Buffer scratch_buffer;
    Buffer as_buffer;
};

struct BLAS : AccelerationStructure {
};

struct TLAS : AccelerationStructure {
    Buffer instance_buffer;
};

struct VulkanApplication {
    private:
    GLFWwindow* window;

    VkInstance vulkan_instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    QueueFamilyIndices queue_family_indices;
    VkDevice logical_device;

    Device device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass render_pass;
    Pipeline pipeline;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

    VkFence immediate_fence;

    VkDebugUtilsMessengerEXT debug_messenger;

    Shaders::CameraData camera_data;
    Buffer camera_buffer;

    MeshData scene_mesh_data;
    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_size_info;
    BLAS scene_blas;
    TLAS scene_tlas;

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

    MeshData create_mesh_data(std::vector<vec4> &vertices, std::vector<uint32_t> &vertex_indices, std::vector<vec4> &normals, std::vector<uint32_t> &normal_indices);

    BLAS build_blas(MeshData &mesh_data);
    void free_blas(BLAS &blas);

    TLAS build_tlas(BLAS &acceleration_structure);
    void free_tlas(TLAS &tlas);

    void setup_device();
    void create_default_descriptor_writes();
    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);
    void create_synchronization();
    void draw_frame();

    public:
    void setup();
    void run();
    void cleanup();
};