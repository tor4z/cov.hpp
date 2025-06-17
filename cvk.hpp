#ifndef CVK_H_
#define CVK_H_

#include <mutex>
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

class VkIns
{
public:
    static void init(const char* app_name);
    static bool create(VkInstance& instance);
private:
    VkApplicationInfo app_info;

    CVK_DEF_SINGLETON(VkIns)
    VkIns();
}; // class VkIns

} // namespace cvk

#endif // CVK_H_

// =======================================
//            Implementation
// =======================================
#define CVK_IMPLEMENTATION


#ifdef CVK_IMPLEMENTATION
namespace cvk {

VkIns::VkIns() {}

void VkIns::init(const char* app_name)
{
    auto ins{instance()};
    ins->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ins->app_info.pApplicationName = app_name;
}

} // namespace cvk

#endif // CVK_IMPLEMENTATION

