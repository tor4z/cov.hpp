#ifndef CVK_H_
#define CVK_H_

#include <cstdint>
#include <iostream>
#include <mutex>
#include <vector>
#include <cstring>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


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
    static VkPhysicalDevice get(const VkInstance& vk_ins);
private:

    CVK_DEF_SINGLETON(PhyDevice);
    PhyDevice();
    static bool is_available(VkPhysicalDevice device);
}; // class PhyDevice

} // namespace cvk

#endif // CVK_H_

// =======================================
//            Implementation
// =======================================
#define CVK_IMPLEMENTATION


#ifdef CVK_IMPLEMENTATION
#ifndef CVK_IMPLEMENTATION_CPP_
#define CVK_IMPLEMENTATION_CPP_

namespace cvk {

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

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &(ins->vk_app_info_);
#ifdef NDEBUG
    info.enabledLayerCount = 0;
#else // NDEBUG
    static const std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    for (auto layer: validation_layers) {
        if (!Extensions::has(layer)) {
            std::cerr << "Validation layer: " << layer << " not found\n";
            return false;
        }
    }

    info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    info.ppEnabledLayerNames = validation_layers.data();
#endif // NDEBUG
    if (vkCreateInstance(&info, nullptr, &vk_instance) != VK_SUCCESS) {
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


VkPhysicalDevice PhyDevice::get(const VkInstance& vk_ins)
{
    uint32_t count{0};
    vkEnumeratePhysicalDevices(vk_ins, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(vk_ins, &count, devices.data());
    
    for (auto device : devices) {
        if (is_available(device)) {
            return device;
        }
    }

    return VK_NULL_HANDLE;
}

bool PhyDevice::is_available(VkPhysicalDevice device)
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

} // namespace cvk

#endif // CVK_IMPLEMENTATION_CPP_
#endif // CVK_IMPLEMENTATION
