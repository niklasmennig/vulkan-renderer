#include "core/vulkan.h"
#include "core/device.h"
#include "core/buffer.h"
#include "loaders/image.h"
#include "loaders/scene.h"
#include "loaders/geometry_gltf.h"
#include "loaders/toml.hpp"
#include "loaders/environment.h"
#include "processors/gltf/gltf_processor.h"
#include "pipeline/raytracing/pipeline_builder.h"
#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"
#include "ui.h"
#include "shader_interface.h"

#include <vector>
#include <optional>
#include <chrono>
#include <functional>
#include <iostream>
#include <filesystem>

#include "glm/glm.hpp"
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

#define NULL_TEXTURE_INDEX 10000 // needs to match index in texture_data.glsl
const std::string camera_data_path = "./camera_data.toml";

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics_compute;
    std::optional<uint32_t> present;
};

struct MeshData {
    VkDevice device_handle;
    size_t index_count;
    size_t vertex_count;
    size_t normal_count;
    size_t texcoord_count;
    size_t tangent_count;
    Buffer indices;
    Buffer vertices;
    Buffer normals;
    Buffer texcoords;
    Buffer tangents;

    void free();
};

struct AccelerationStructure {
    VkAccelerationStructureKHR acceleration_structure;
    Buffer buffer;
};

struct VulkanApplication {
    private:
    GLFWwindow* window;
    std::chrono::time_point<std::chrono::high_resolution_clock> startup_time, last_frame_time;
    std::chrono::duration<double> frame_delta;

    bool minimized = false;

    float last_cursor_x, last_cursor_y, delta_cursor_x, delta_cursor_y;

    VkInstance vulkan_instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    QueueFamilyIndices queue_family_indices;
    VkDevice logical_device;

    Device device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    UI ui;

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkDescriptorPool imgui_descriptor_pool;

    VkExtent2D render_image_extent;

    VkRenderPass render_pass;
    RaytracingPipelineBuilder rt_pipeline_builder;
    RaytracingPipeline rt_pipeline;

    ProcessingPipelineBuilder p_pipeline_builder;
    ProcessingPipeline p_pipeline;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

    VkFence immediate_fence, tlas_fence;

    VkDebugUtilsMessengerEXT debug_messenger;

    float camera_look_x, camera_look_y;
    bool mouse_look_active = false;

    Shaders::CameraData camera_data;
    Buffer camera_buffer;
    bool camera_changed;

    uint32_t render_clear_accumulated;

    bool render_images_dirty = true;
    bool pipeline_dirty = false;

    std::filesystem::path scene_path;
    SceneData loaded_scene_data;

    EnvironmentMap loaded_environment;

    GLTFProcessor* gltf_processor = nullptr;

    // these use loaded_mesh_index
    std::vector<MeshData> created_meshes;
    std::vector<AccelerationStructure> created_blas;

    // this uses loaded_texture_index
    std::vector<Image> loaded_textures;

    // mapping object name -> GLTF data
    std::unordered_map<std::string, GLTFData> loaded_objects;
    // mapping object name -> mesh index offset
    std::unordered_map<std::string, uint32_t> loaded_mesh_index;
    // mapping object name -> texture index offset
    std::unordered_map<std::string, uint32_t> loaded_texture_index;

    AccelerationStructure scene_tlas;

    Buffer index_buffer, vertex_buffer, normal_buffer, texcoord_buffer, tangent_buffer, mesh_data_offset_buffer, mesh_offset_index_buffer, texture_index_buffer, material_parameter_buffer;
    Buffer lights_buffer;
    std::vector<InstanceData::MaterialParameters> material_parameters;
    std::vector<Shaders::Light> lights;

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

    MeshData create_mesh_data(std::vector<uint32_t> &indices, std::vector<vec3> &vertices, std::vector<vec3> &normals, std::vector<vec2> &texcoords, std::vector<vec3> &tangents);

    AccelerationStructure build_blas(std::vector<uint32_t> &indices, std::vector<vec3> &vertices, uint32_t max_vertex);
    AccelerationStructure build_tlas();

    void submit_immediate(std::function<void()> lambda);

    void setup_device();
    void init_imgui();
    void create_swapchain();
    void create_swapchain_image_views();
    void create_framebuffers();
    void recreate_swapchain();
    void recreate_render_images();
    void rebuild_pipeline();
    void create_default_descriptor_writes();
    void create_synchronization();
    void draw_frame();

    public:

    void set_scene_path(std::string path);
    void setup();
    void run();
    void cleanup();

    void set_render_images_dirty();
    void set_pipeline_dirty();

    RaytracingPipeline get_pipeline();
    SceneData& get_scene_data();
    std::vector<Shaders::Light>& get_lights();

    void save_screenshot(std::string path, ImagePixels& pixels);

    double get_fps();
    uint32_t get_samples();
    vec2 get_cursor_position();
};