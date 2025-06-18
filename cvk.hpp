#ifndef CVK_H_
#define CVK_H_

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <mutex>

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


namespace cvk {

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

Device::Device() {}

bool Device::create(VkPhysicalDevice phy_device, uint32_t queue_index, VkDevice& device, VkQueue& queue)
{
    float queue_priority{1.0f}; // 0.0f~1.0f

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

} // namespace cvk

#endif // CVK_IMPLEMENTATION_CPP_
#endif // CVK_IMPLEMENTATION
