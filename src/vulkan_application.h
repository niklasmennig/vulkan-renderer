#include "vulkan.h"
#include "device.h"
#include "buffer.h"
#include "loaders/image.h"
#include "loaders/scene.h"
#include "loaders/geometry_obj.h"
#include "pipeline_builder.h"

#include <vector>
#include <optional>
#include <chrono>

#include "glm/glm.hpp"
using vec2 = glm::vec2;
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
    uint32_t texcoord_count;
    uint32_t vertex_index_count;
    uint32_t normal_index_count;
    uint32_t texcoord_index_count;
    Buffer vertices;
    Buffer vertex_indices;
    Buffer normals;
    Buffer normal_indices;
    Buffer texcoords;
    Buffer texcoord_indices;

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
    std::chrono::time_point<std::chrono::high_resolution_clock> startup_time, last_frame_time;
    std::chrono::duration<double> frame_delta;

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

    int32_t render_clear_accumulated = 4;


    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_size_info;

    SceneData loaded_scene_data;
    std::vector<LoadedMeshData> loaded_mesh_data;
    std::unordered_map<std::string, uint32_t> loaded_mesh_index;
    std::vector<MeshData> created_meshes;
    std::vector<Image> loaded_textures;
    std::unordered_map<std::string, uint32_t> loaded_texture_index;

    std::unordered_map<std::string, BLAS> loaded_blas;
    TLAS scene_tlas;

    Buffer vertex_buffer, vertex_index_buffer, normal_buffer, normal_index_buffer, texcoord_buffer, texcoord_index_buffer, mesh_data_offset_buffer, mesh_offset_index_buffer, texture_index_buffer;

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

    MeshData create_mesh_data(std::vector<vec4> &vertices, std::vector<uint32_t> &vertex_indices, std::vector<vec4> &normals, std::vector<uint32_t> &normal_indices, std::vector<vec2> &texcoords, std::vector<uint32_t> &texcoord_indices);
    MeshData create_mesh_data(LoadedMeshData loaded_mesh_data);

    BLAS build_blas(MeshData &mesh_data);
    void free_blas(BLAS &blas);

    TLAS build_tlas();
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