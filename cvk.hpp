#ifndef CVK_H_
#define CVK_H_

#include <cstdint>
#include <iostream>
#include <mutex>
#include <vector>
#include <cstring>
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


namespace cvk {

class VkExts
{
public:
    static const std::vector<VkExtensionProperties>& get();
    static bool has_ext(const char* ext_name);
private:
    std::vector<VkExtensionProperties> exts_;

    CVK_DEF_SINGLETON(VkExts)
    VkExts();
}; // class VkExts

class VkIns
{
public:
    static void init(const char* app_name);
    static bool create(VkInstance& instance);
    static void destroy(const VkInstance& instance);
    static void destroy_all();
private:
    VkApplicationInfo vk_app_info_;
    std::vector<VkInstance> vk_ins_;

    CVK_DEF_SINGLETON(VkIns)
    VkIns();
    ~VkIns();
}; // class VkIns


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

VkIns::VkIns() {}

VkIns::~VkIns() {}

void VkIns::init(const char* app_name)
{
    auto ins{instance()};
    ins->vk_app_info_.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ins->vk_app_info_.pApplicationName = app_name;
    ins->vk_app_info_.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ins->vk_app_info_.pEngineName = "No Engine";
    ins->vk_app_info_.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ins->vk_app_info_.apiVersion = VK_API_VERSION_1_4;
}

bool VkIns::create(VkInstance& vk_instance)
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
        if (!VkExts::has_ext(layer)) {
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

void VkIns::destroy(const VkInstance& instance)
{
    vkDestroyInstance(instance, nullptr);
}

void VkIns::destroy_all()
{
    auto ins{instance()};
    for (const auto& vk_ins: ins->vk_ins_) {
        destroy(vk_ins);
    }
}

VkExts::VkExts()
{
    uint32_t count;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    exts_.resize(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, exts_.data());
}

const std::vector<VkExtensionProperties>& VkExts::get()
{
    return instance()->exts_;
}

bool VkExts::has_ext(const char* ext_name)
{
    const auto& exts{get()};
    for (const auto& ext: exts) {
        if (std::strcmp(ext_name, ext.extensionName) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace cvk

#endif // CVK_IMPLEMENTATION_CPP_
#endif // CVK_IMPLEMENTATION

