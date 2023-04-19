#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
};

struct Buffer {
    size_t buffer_size;
    VkBuffer buffer_handle;
    VkDeviceMemory device_memory;
    VkDeviceAddress device_address;
};

struct ShaderBindingTable {
    Buffer raygen;
    Buffer hit;
    Buffer miss;
};

struct VulkanApplication {
    private:
    GLFWwindow* window;

    VkInstance vulkan_instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    QueueFamilyIndices queue_family_indices;
    VkDevice logical_device;

    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass render_pass;
    VkPipelineCache pipeline_cache;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    ShaderBindingTable shader_binding_table;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

    VkDebugUtilsMessengerEXT debug_messenger;

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

    Buffer create_buffer(VkBufferCreateInfo* create_info);
    void set_buffer_data(Buffer &buffer, void* data);
    void free_buffer(Buffer &buffer);

    VkShaderModule create_shader_module(const std::vector<char> &shader_code);
    void create_descriptor_set_layout();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_graphics_pipeline();
    void create_raytracing_pipeline();
    ShaderBindingTable create_shader_binding_table();
    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);
    void create_synchronization();
    void draw_frame();

    public:
    void setup();
    void run();
    void cleanup();
};