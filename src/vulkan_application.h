#include "vulkan.h"
#include "device.h"
#include "buffer.h"
#include "loaders/image.h"
#include "loaders/scene.h"
#include "loaders/geometry_obj.h"
#include "loaders/geometry_gltf.h"
#include "pipeline_builder.h"
#include "glsl_compiler.h"
#include "ui.h"

#include <vector>
#include <optional>
#include <chrono>
#include <functional>

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
    size_t index_count;
    size_t vertex_count;
    size_t normal_count;
    size_t texcoord_count;
    Buffer indices;
    Buffer vertices;
    Buffer normals;
    Buffer texcoords;

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

    GLSLCompiler glsl_compiler;

    VkInstance vulkan_instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    QueueFamilyIndices queue_family_indices;
    VkDevice logical_device;

    Device device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    Image render_image, aov_indices;

    UI ui;

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkDescriptorPool imgui_descriptor_pool;

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

    uint32_t render_clear_accumulated = 4;


    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_size_info;

    SceneData loaded_scene_data;
    std::vector<LoadedMeshData> loaded_mesh_data;
    std::unordered_map<std::string, uint32_t> loaded_mesh_index;
    std::unordered_map<std::string, GLTFData> loaded_objects;
    std::vector<MeshData> created_meshes;
    std::vector<Image> loaded_textures;
    std::unordered_map<std::string, uint32_t> loaded_texture_index;

    std::unordered_map<std::string, BLAS> loaded_blas;
    TLAS scene_tlas;

    Buffer index_buffer, vertex_buffer, normal_buffer, texcoord_buffer, mesh_data_offset_buffer, mesh_offset_index_buffer, texture_index_buffer, material_parameter_buffer;
    std::vector<float> material_parameters;

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

    MeshData create_mesh_data(std::vector<uint32_t> &indices, std::vector<vec4> &vertices, std::vector<vec4> &normals, std::vector<vec2> &texcoords);
    MeshData create_mesh_data(LoadedMeshData loaded_mesh_data);

    BLAS build_blas(MeshData &mesh_data);
    void free_blas(BLAS &blas);

    TLAS build_tlas();
    void free_tlas(TLAS &tlas);

    void submit_immediate(std::function<void()> lambda);

    void setup_device();
    void init_imgui();
    void create_swapchain();
    void create_swapchain_image_views();
    void create_framebuffers();
    void recreate_swapchain();
    void create_render_image();
    void recreate_render_image();
    void create_default_descriptor_writes();
    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);
    void create_synchronization();
    void draw_frame();

    public:
    void setup();
    void run();
    void cleanup();

    double get_fps();
    uint32_t get_samples();
};