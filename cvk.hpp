#ifndef CVK_H_
#define CVK_H_

#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>
#include <mutex>
#include <cstring>
#include <iostream>
#include <vulkan/vulkan.h>

#define CVK_DEF_SINGLETON(classname)                        \
    static inline classname* instance()                     \
    {                                                       \
        static classname *instance_ = nullptr;              \
        static std::once_flag flag;                         \
        if (!instance_) {                                   \
            std::call_once(flag, [&](){                     \
                instance_ = new (std::nothrow) classname(); \
            });                                             \
        }                                                   \
        return instance_;                                   \
    }                                                       \
private:                                                    \
    classname(const classname&) = delete;                   \
    classname& operator=(const classname&) = delete;        \
    classname(const classname&&) = delete;                  \
    classname& operator=(const classname&&) = delete;

#define CVK_CHECK_ASSERT(result)                                                \
    if ((result) != VK_SUCCESS) {                                               \
        std::cerr << "Vulkan Failed with error: " << stringify(result) << "\n"; \
        assert(false);                                                          \
    }

namespace cvk {

std::string stringify(VkResult result);

class Extensions
{
public:
    static const std::vector<VkExtensionProperties>& get();
    static bool has(const char* ext_name);
private:
    std::vector<VkExtensionProperties> exts_;

    CVK_DEF_SINGLETON(Extensions)
    Extensions();
}; // class Extensions

class App
{
public:
    static void init(const char* app_name);
    static bool create_instance(VkInstance& instance);
    static void destroy_instance(const VkInstance& instance);
    static void destroy_all_instance();
private:
    VkApplicationInfo vk_app_info_;
    std::vector<VkInstance> vk_ins_;

    CVK_DEF_SINGLETON(App)
    App();
    ~App();
}; // class App

class PhyDevice
{
public:
    static bool get(const VkInstance& vk_ins, VkPhysicalDevice& device, uint32_t& queue_index);
    static void properties(VkPhysicalDevice device, VkPhysicalDeviceProperties& properties);
private:
    CVK_DEF_SINGLETON(PhyDevice);
    PhyDevice();
    static std::optional<uint32_t> find_available_queue(VkPhysicalDevice device);
    static bool property_available(VkPhysicalDevice device);
}; // class PhyDevice

class Device
{
public:
    static bool create(VkPhysicalDevice phy_device, uint32_t queue_index, VkDevice& device, VkQueue& queue);
    static void destroy(VkDevice device);
private:
    CVK_DEF_SINGLETON(Device)

    Device();
}; // class Device


class CommandPool
{
public:
    static bool create(VkDevice device, uint32_t queue_index, VkCommandPool& cmd_pool);
}; // class CommandPool


bool create_buffer(VkDevice device, VkPhysicalDevice phy_device, size_t size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags property_flags, VkBuffer& buff, VkDeviceMemory& memory)
{
    // create buffer
    VkBufferCreateInfo buff_create_info{};
    buff_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_create_info.size = size;
    buff_create_info.usage = usage;
    buff_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CVK_CHECK_ASSERT(vkCreateBuffer(device, &buff_create_info, nullptr, &buff))

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, buff, &mem_reqs);

    VkMemoryAllocateInfo mem_alloc_info{};
    mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.allocationSize = mem_reqs.size;

    VkPhysicalDeviceMemoryProperties mem_properies;
    vkGetPhysicalDeviceMemoryProperties(phy_device, &mem_properies);

    bool mem_satisfied{false};
    for (int i = 0; i < mem_properies.memoryTypeCount; ++i) {
        if ((mem_reqs.memoryTypeBits & 0x1) &&
            (mem_properies.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
            mem_satisfied = true;
            mem_alloc_info.memoryTypeIndex = i;
            break;
        }
        mem_reqs.memoryTypeBits >>= 1;
    }

    if (!mem_satisfied) {
        return false;
    }

    CVK_CHECK_ASSERT(vkAllocateMemory(device, &mem_alloc_info, nullptr, &memory))
    CVK_CHECK_ASSERT(vkBindBufferMemory(device, buff, memory, 0))
    return true;
}


template<typename T>
bool to_device(const T* data, size_t size, VkDevice device,
    VkPhysicalDevice phy_device, VkCommandPool cmd_pool, VkQueue queue)
{
    VkBuffer host_buff;
    VkDeviceMemory host_memory;
    VkBuffer device_buff;
    VkDeviceMemory device_memory;

    create_buffer(device, phy_device, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, host_buff, host_memory);

    if (data) {
        void* mapped_data;
        vkMapMemory(device, host_memory, 0, size, 0, &mapped_data);
        memcpy(mapped_data, reinterpret_cast<const void*>(data), size);
        vkUnmapMemory(device, host_memory);
    }

    VkMappedMemoryRange mem_range{};
    mem_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mem_range.size = size;
    mem_range.offset = 0;
    mem_range.memory = host_memory;
    CVK_CHECK_ASSERT(vkFlushMappedMemoryRanges(device, 1, &mem_range));
    vkUnmapMemory(device, host_memory);

    create_buffer(device, phy_device, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device_buff, device_memory);

    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.commandPool = cmd_pool;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buff;
    CVK_CHECK_ASSERT(vkAllocateCommandBuffers(device, &cmd_alloc_info, &cmd_buff));

    VkCommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CVK_CHECK_ASSERT(vkBeginCommandBuffer(cmd_buff, &cmd_begin_info));
    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buff, host_buff, device_buff, 1, &copy_region);
    CVK_CHECK_ASSERT(vkEndCommandBuffer(cmd_buff));

    // submmit
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buff;

    VkFenceCreateInfo fence_info{};
    VkFence fence{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;
    CVK_CHECK_ASSERT(vkCreateFence(device, &fence_info, nullptr, &fence))
    CVK_CHECK_ASSERT(vkQueueSubmit(queue, 1, &submit_info, fence))
    CVK_CHECK_ASSERT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX))

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buff);
    return true;
}

} // namespace cvk

#endif // CVK_H_

// =======================================
//            Implementation
// =======================================
#define CVK_IMPLEMENTATION


#ifdef CVK_IMPLEMENTATION
#ifndef CVK_IMPLEMENTATION_CPP_
#define CVK_IMPLEMENTATION_CPP_

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <cstring>


#ifdef NDEBUG
#   define CVK_ENABLE_VALIDATION 0
#else
#   define CVK_ENABLE_VALIDATION 1
#endif

#define CHECK_VALIDATION_AVAILABLE()                                            \
    do {                                                                        \
        for (auto layer: vck_validation_layers) {                               \
            if (!Extensions::has(layer)) {                                      \
                std::cerr << "Validation layer: " << layer << " not found\n";   \
                exit(1);                                                        \
            }                                                                   \
        }                                                                       \
    } while (0)


namespace cvk {

const std::vector<const char*> vck_validation_layers = {
    "VK_LAYER_KHRONOS_validation"
}; // vck_validation_layers

std::string stringify(VkResult result)
{
#define CVK_MATCH_STRINGIFY(r) case VK_##r: return #r;

    switch (result) {
    CVK_MATCH_STRINGIFY(SUCCESS)
    CVK_MATCH_STRINGIFY(NOT_READY)
    CVK_MATCH_STRINGIFY(TIMEOUT)
    CVK_MATCH_STRINGIFY(EVENT_SET)
    CVK_MATCH_STRINGIFY(EVENT_RESET)
    CVK_MATCH_STRINGIFY(INCOMPLETE)
    CVK_MATCH_STRINGIFY(ERROR_OUT_OF_HOST_MEMORY)
    CVK_MATCH_STRINGIFY(ERROR_OUT_OF_DEVICE_MEMORY)
    CVK_MATCH_STRINGIFY(ERROR_INITIALIZATION_FAILED)
    CVK_MATCH_STRINGIFY(ERROR_DEVICE_LOST)
    CVK_MATCH_STRINGIFY(ERROR_MEMORY_MAP_FAILED)
    CVK_MATCH_STRINGIFY(ERROR_LAYER_NOT_PRESENT)
    CVK_MATCH_STRINGIFY(ERROR_EXTENSION_NOT_PRESENT)
    CVK_MATCH_STRINGIFY(ERROR_FEATURE_NOT_PRESENT)
    CVK_MATCH_STRINGIFY(ERROR_INCOMPATIBLE_DRIVER)
    CVK_MATCH_STRINGIFY(ERROR_TOO_MANY_OBJECTS)
    CVK_MATCH_STRINGIFY(ERROR_FORMAT_NOT_SUPPORTED)
    CVK_MATCH_STRINGIFY(ERROR_FRAGMENTED_POOL)
    CVK_MATCH_STRINGIFY(ERROR_UNKNOWN)
    CVK_MATCH_STRINGIFY(ERROR_OUT_OF_POOL_MEMORY)
    CVK_MATCH_STRINGIFY(ERROR_INVALID_EXTERNAL_HANDLE)
    CVK_MATCH_STRINGIFY(ERROR_FRAGMENTATION)
    CVK_MATCH_STRINGIFY(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
    CVK_MATCH_STRINGIFY(PIPELINE_COMPILE_REQUIRED)
    CVK_MATCH_STRINGIFY(ERROR_NOT_PERMITTED)
    CVK_MATCH_STRINGIFY(ERROR_SURFACE_LOST_KHR)
    CVK_MATCH_STRINGIFY(ERROR_NATIVE_WINDOW_IN_USE_KHR)
    CVK_MATCH_STRINGIFY(SUBOPTIMAL_KHR)
    CVK_MATCH_STRINGIFY(ERROR_OUT_OF_DATE_KHR)
    CVK_MATCH_STRINGIFY(ERROR_INCOMPATIBLE_DISPLAY_KHR)
    CVK_MATCH_STRINGIFY(ERROR_VALIDATION_FAILED_EXT)
    CVK_MATCH_STRINGIFY(ERROR_INVALID_SHADER_NV)
    CVK_MATCH_STRINGIFY(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
    CVK_MATCH_STRINGIFY(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
    CVK_MATCH_STRINGIFY(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
    CVK_MATCH_STRINGIFY(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
    CVK_MATCH_STRINGIFY(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
    CVK_MATCH_STRINGIFY(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
    CVK_MATCH_STRINGIFY(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
    CVK_MATCH_STRINGIFY(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
    CVK_MATCH_STRINGIFY(THREAD_IDLE_KHR)
    CVK_MATCH_STRINGIFY(THREAD_DONE_KHR)
    CVK_MATCH_STRINGIFY(OPERATION_DEFERRED_KHR)
    CVK_MATCH_STRINGIFY(OPERATION_NOT_DEFERRED_KHR)
    CVK_MATCH_STRINGIFY(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
    CVK_MATCH_STRINGIFY(ERROR_COMPRESSION_EXHAUSTED_EXT)
    CVK_MATCH_STRINGIFY(INCOMPATIBLE_SHADER_BINARY_EXT)
    CVK_MATCH_STRINGIFY(PIPELINE_BINARY_MISSING_KHR)
    CVK_MATCH_STRINGIFY(ERROR_NOT_ENOUGH_SPACE_KHR)
    default: return "UNKNOWN ERROR";
    }
#undef CVK_MATCH_STRINGIFY
}

App::App() {}

App::~App() {}

void App::init(const char* app_name)
{
    auto ins{instance()};
    ins->vk_app_info_.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ins->vk_app_info_.pApplicationName = app_name;
    ins->vk_app_info_.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ins->vk_app_info_.pEngineName = "No Engine";
    ins->vk_app_info_.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ins->vk_app_info_.apiVersion = VK_API_VERSION_1_4;
}

bool App::create_instance(VkInstance& vk_instance)
{
    auto ins{instance()};

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &(ins->vk_app_info_);
#if CVK_ENABLE_VALIDATION
    CHECK_VALIDATION_AVAILABLE();
    create_info.enabledLayerCount = static_cast<uint32_t>(vck_validation_layers.size());
    create_info.ppEnabledLayerNames = vck_validation_layers.data();
#else // CVK_ENABLE_VALIDATION
    create_info.enabledLayerCount = 0;
#endif // CVK_ENABLE_VALIDATION
    if (vkCreateInstance(&create_info, nullptr, &vk_instance) != VK_SUCCESS) {
        return false;
    }

    ins->vk_ins_.push_back(vk_instance);
    return true;
}

void App::destroy_instance(const VkInstance& instance)
{
    vkDestroyInstance(instance, nullptr);
}

void App::destroy_all_instance()
{
    auto ins{instance()};
    for (const auto& vk_ins: ins->vk_ins_) {
        destroy_instance(vk_ins);
    }
}

Extensions::Extensions()
{
    uint32_t count;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    exts_.resize(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, exts_.data());
}

const std::vector<VkExtensionProperties>& Extensions::get()
{
    return instance()->exts_;
}

bool Extensions::has(const char* ext_name)
{
    const auto& exts{get()};
    for (const auto& ext: exts) {
        if (std::strcmp(ext_name, ext.extensionName) == 0) {
            return true;
        }
    }
    return false;
}


PhyDevice::PhyDevice() {}


bool PhyDevice::get(const VkInstance& vk_ins, VkPhysicalDevice& device, uint32_t& que_family_index)
{
    uint32_t count{0};
    vkEnumeratePhysicalDevices(vk_ins, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(vk_ins, &count, devices.data());

    std::optional<uint32_t> queue_index;
    for (auto dev : devices) {
        if (property_available(dev)) {
            queue_index = find_available_queue(dev);
            if (queue_index.has_value()) {
                device = dev;
                que_family_index = queue_index.value();
                return true;
            }
        }
    }

    device = VK_NULL_HANDLE;
    return false;
}

bool PhyDevice::property_available(VkPhysicalDevice device)
{
    if (device == VK_NULL_HANDLE) {
        return false;
    }

    // VkPhysicalDeviceProperties properties;
    // VkPhysicalDeviceFeatures features;

    // vkGetPhysicalDeviceProperties(device, &properties);
    // vkGetPhysicalDeviceFeatures(device, &features);

    return true;
}

std::optional<uint32_t> PhyDevice::find_available_queue(VkPhysicalDevice device)
{
    if (device == VK_NULL_HANDLE) {
        return false;
    }

    std::optional<uint32_t> result;
    uint32_t que_family_count{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &que_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> que_family_properties(que_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &que_family_count, que_family_properties.data());

    for (int i = 0; i < que_family_properties.size(); ++i) {
        const auto& que{que_family_properties.at(i)};
        if (que.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            result = i;
            break;
        }
    }

    return result;
}

void PhyDevice::properties(VkPhysicalDevice device, VkPhysicalDeviceProperties& properties)
{
    vkGetPhysicalDeviceProperties(device, &properties);
}

Device::Device() {}

bool Device::create(VkPhysicalDevice phy_device, uint32_t queue_index, VkDevice& device, VkQueue& queue)
{
    const float queue_priority{1.0f}; // 0.0f~1.0f

    VkDeviceQueueCreateInfo que_create_info{};
    que_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    que_create_info.queueCount = 1;
    que_create_info.queueFamilyIndex = queue_index;
    que_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = &que_create_info;
    create_info.queueCreateInfoCount = 1;
    create_info.pEnabledFeatures = &device_features;
#if CVK_ENABLE_VALIDATION
    CHECK_VALIDATION_AVAILABLE();
    create_info.ppEnabledLayerNames = vck_validation_layers.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(vck_validation_layers.size());
#else // CVK_ENABLE_VALIDATION
    create_info.enabledLayerCount = 0;
#endif // CVK_ENABLE_VALIDATION

    if (vkCreateDevice(phy_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(device, queue_index, 0, &queue);
    return true;
}

void Device::destroy(VkDevice device)
{
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
    }
}

bool CommandPool::create(VkDevice device, uint32_t queue_index, VkCommandPool& cmd_pool)
{
    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.queueFamilyIndex = queue_index;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &create_info, nullptr, &cmd_pool) != VK_SUCCESS) {
        return false;
    }
    return true;
}

} // namespace cvk

#endif // CVK_IMPLEMENTATION_CPP_
#endif // CVK_IMPLEMENTATION
