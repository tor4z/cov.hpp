#ifndef COV_H_
#define COV_H_

#include <mutex>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define COV_DEF_SINGLETON(classname)                                            \
    static inline classname* instance()                                         \
    {                                                                           \
        static classname *instance_ = nullptr;                                  \
        static std::once_flag flag;                                             \
        if (!instance_) {                                                       \
            std::call_once(flag, [&](){                                         \
                instance_ = new (std::nothrow) classname();                     \
            });                                                                 \
        }                                                                       \
        return instance_;                                                       \
    }                                                                           \
private:                                                                        \
    classname(const classname&) = delete;                                       \
    classname& operator=(const classname&) = delete;                            \
    classname(const classname&&) = delete;                                      \
    classname& operator=(const classname&&) = delete;

namespace cov {

class PhysicalDevice
{
public:
    PhysicalDevice();
    bool get(const VkInstance& vk_ins, VkPhysicalDevice& device, uint32_t& queue_index);
    void properties(VkPhysicalDevice device, VkPhysicalDeviceProperties& properties);
private:
    static std::optional<uint32_t> find_available_queue(VkPhysicalDevice device);
    static bool property_available(VkPhysicalDevice device);
}; // class PhyDevice

class Device
{
public:
    Device();
    bool create(VkPhysicalDevice phy_device, uint32_t queue_index, VkDevice& device, VkQueue& queue);
    void destroy(VkDevice device);
private:
}; // class Device

class Instance
{
public:
    ~Instance() { destroy(); }
    // not copiable
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;
    // movable
    Instance(Instance&&);
    Instance& operator=(Instance&&);

    bool to_device(const void* data, size_t size);
    bool to_host(void* data, size_t size);
    bool load_shader(const std::string& shader_path, void* spec_data=nullptr, size_t spec_data_len=0);
    void add_spec_item(uint32_t const_id, uint32_t offset, size_t item_size);
    bool execute(int dims[3]);
    void destroy();
private:
    VkInstance vk_instance_;
    VkCommandPool cmd_pool_;
    VkBuffer host_buff_;
    VkDeviceMemory host_memory_;
    VkBuffer device_buff_;
    VkDeviceMemory device_memory_;
    VkQueue queue_;
    VkPhysicalDevice phy_device_;
    VkDevice device_;
    VkShaderModule shader_module_;
    VkSpecializationInfo spec_info_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    std::vector<VkSpecializationMapEntry> spec_map_entryies_;
    uint32_t queue_index_;

    friend class App;
    Instance(VkInstance vk_instance);
    static bool init_command_pool(VkDevice device, uint32_t queue_index, VkCommandPool& cmd_pool);
}; // class Instance

class App
{
public:
    static void init(const char* app_name);
    static Instance new_instance();
private:
    VkApplicationInfo vk_app_info_;

    COV_DEF_SINGLETON(App)
    App() = default;
    ~App() = default;
}; // class App

} // namespace cov

#endif // COV_H_

// =======================================
//            Implementation
// =======================================
#define COV_IMPLEMENTATION // please delete me


#ifdef COV_IMPLEMENTATION
#ifndef COV_IMPLEMENTATION_CPP_
#define COV_IMPLEMENTATION_CPP_

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <ios>
#include <vector>
#include <mutex>
#include <cstring>
#include <iostream>

#ifdef COV_VULKAN_VALIDATION
#   define COV_ENABLE_VALIDATION 1
#else // COV_VULKAN_VALIDATION
#   define COV_ENABLE_VALIDATION 0
#endif // COV_VULKAN_VALIDATION


#define CHECK_VALIDATION_AVAILABLE()                                            \
    do {                                                                        \
        for (auto layer: cov_validation_layers) {                               \
            if (!LayerExtensions::has_layer(layer)) {                                      \
                std::cerr << "Validation layer: " << layer << " not found\n";   \
                exit(1);                                                        \
            }                                                                   \
        }                                                                       \
    } while (0)

#define COV_CHECK_ASSERT(result)                                                \
    if ((result) != VK_SUCCESS) {                                               \
        std::cerr << "Vulkan Failed with error: " << stringify(result) << "\n"; \
        assert(false);                                                          \
    }

#define COV_CHECK_FALSE(result)                                                 \
    if ((result) != VK_SUCCESS) {                                               \
        std::cerr << "Vulkan Failed with error: " << stringify(result) << "\n"; \
        return false;                                                           \
    }

namespace cov {

const std::vector<const char*> cov_validation_layers = {
    "VK_LAYER_KHRONOS_validation"
}; // cov_validation_layers

std::string stringify(VkResult result);

bool create_buffer(VkDevice device, VkPhysicalDevice phy_device, size_t size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags property_flags, VkBuffer& buff, VkDeviceMemory& memory);

class LayerExtensions
{
public:
    static const std::vector<VkExtensionProperties>& get_exts();
    static const std::vector<VkLayerProperties>& get_layers();
    static bool has_ext(const char* ext_name);
    static bool has_layer(const char* layer_name);
private:
    std::vector<VkExtensionProperties> exts_;
    std::vector<VkLayerProperties> layers_;

    COV_DEF_SINGLETON(LayerExtensions)
    LayerExtensions();
}; // class LayerExtensions

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

Instance App::new_instance()
{
    auto ins{instance()};

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &(ins->vk_app_info_);
#if COV_ENABLE_VALIDATION
    CHECK_VALIDATION_AVAILABLE();
    create_info.enabledLayerCount = static_cast<uint32_t>(cov_validation_layers.size());
    create_info.ppEnabledLayerNames = cov_validation_layers.data();
#else // COV_ENABLE_VALIDATION
    create_info.enabledLayerCount = 0;
#endif // COV_ENABLE_VALIDATION

    VkInstance vk_instance;
    COV_CHECK_ASSERT(vkCreateInstance(&create_info, nullptr, &vk_instance))
    return Instance(vk_instance);
}

Instance::Instance(VkInstance vk_instance)
    : vk_instance_(vk_instance)
    , cmd_pool_(VK_NULL_HANDLE)
    , host_buff_(VK_NULL_HANDLE)
    , host_memory_(VK_NULL_HANDLE)
    , device_buff_(VK_NULL_HANDLE)
    , device_memory_(VK_NULL_HANDLE)
    , queue_(VK_NULL_HANDLE)
    , phy_device_(VK_NULL_HANDLE)
    , device_(VK_NULL_HANDLE)
    , shader_module_(VK_NULL_HANDLE)
    , queue_index_(-1)
{
    PhysicalDevice physical_device_creator;
    Device device_creator;

    physical_device_creator.get(vk_instance_, phy_device_, queue_index_);
    device_creator.create(phy_device_, queue_index_, device_, queue_);
    init_command_pool(device_, queue_index_, cmd_pool_);

    spec_info_.dataSize = 0;
    spec_info_.pData = nullptr;
    spec_info_.mapEntryCount = 0;
    spec_info_.pMapEntries = nullptr;
}

Instance::Instance(Instance&& other)
{
    vk_instance_ = other.vk_instance_;
    cmd_pool_ = other.cmd_pool_;
    host_buff_ = other.host_buff_;
    host_memory_ = other.host_memory_;
    device_buff_ = other.device_buff_;
    device_memory_ = other.device_memory_;
    queue_ = other.queue_;
    phy_device_ = other.phy_device_;
    device_ = other.device_;
    shader_module_ = other.shader_module_;
    queue_index_ = other.queue_index_;
    spec_info_ = other.spec_info_;
    spec_map_entryies_ = other.spec_map_entryies_;

    other.vk_instance_ = VK_NULL_HANDLE;
    other.cmd_pool_ = VK_NULL_HANDLE;
    other.host_buff_ = VK_NULL_HANDLE;
    other.host_memory_ = VK_NULL_HANDLE;
    other.device_buff_ = VK_NULL_HANDLE;
    other.device_memory_ = VK_NULL_HANDLE;
    other.queue_ = VK_NULL_HANDLE;
    other.phy_device_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.shader_module_ = VK_NULL_HANDLE;
    other.queue_index_ = -1;
    other.spec_map_entryies_.clear();
}

Instance& Instance::operator=(Instance&& other)
{
    if (this == &other) {
        return *this;
    }

    destroy();
    vk_instance_ = other.vk_instance_;
    cmd_pool_ = other.cmd_pool_;
    host_buff_ = other.host_buff_;
    host_memory_ = other.host_memory_;
    device_buff_ = other.device_buff_;
    device_memory_ = other.device_memory_;
    queue_ = other.queue_;
    phy_device_ = other.phy_device_;
    device_ = other.device_;
    shader_module_ = other.shader_module_;
    queue_index_ = other.queue_index_;
    spec_info_ = other.spec_info_;
    spec_map_entryies_ = other.spec_map_entryies_;

    other.vk_instance_ = VK_NULL_HANDLE;
    other.cmd_pool_ = VK_NULL_HANDLE;
    other.host_buff_ = VK_NULL_HANDLE;
    other.host_memory_ = VK_NULL_HANDLE;
    other.device_buff_ = VK_NULL_HANDLE;
    other.device_memory_ = VK_NULL_HANDLE;
    other.queue_ = VK_NULL_HANDLE;
    other.phy_device_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.shader_module_ = VK_NULL_HANDLE;
    other.queue_index_ = -1;
    other.spec_map_entryies_.clear();

    return *this;
}

void Instance::destroy()
{
    if (device_ && cmd_pool_) {
        vkDestroyCommandPool(device_, cmd_pool_, nullptr);
    }

    if (host_buff_) {
        vkDestroyBuffer(device_, host_buff_, nullptr);
    }

    if (device_buff_) {
        vkDestroyBuffer(device_, device_buff_, nullptr);
    }

    if (host_memory_) {
        vkFreeMemory(device_, host_memory_, nullptr);
    }

    if (device_memory_) {
        vkFreeMemory(device_, device_memory_, nullptr);
    }

    if (shader_module_) {
        vkDestroyShaderModule(device_, shader_module_, nullptr);
    }

    if (device_) {
        vkDestroyDevice(device_, nullptr);
    }
    if (vk_instance_) {
        vkDestroyInstance(vk_instance_, nullptr);
    }

    spec_map_entryies_.clear();
}

bool Instance::to_device(const void* data, size_t size)
{
    create_buffer(device_, phy_device_, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, host_buff_, host_memory_);

    if (data) {
        void* mapped_data;
        vkMapMemory(device_, host_memory_, 0, size, 0, &mapped_data);
        memcpy(mapped_data, reinterpret_cast<const void*>(data), size);

        VkMappedMemoryRange mem_range{};
        mem_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mem_range.size = size;
        mem_range.offset = 0;
        mem_range.memory = host_memory_;
        COV_CHECK_ASSERT(vkFlushMappedMemoryRanges(device_, 1, &mem_range));
        vkUnmapMemory(device_, host_memory_);
    }

    create_buffer(device_, phy_device_, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device_buff_, device_memory_);

    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.commandPool = cmd_pool_;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buff;
    COV_CHECK_ASSERT(vkAllocateCommandBuffers(device_, &cmd_alloc_info, &cmd_buff));

    VkCommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    COV_CHECK_ASSERT(vkBeginCommandBuffer(cmd_buff, &cmd_begin_info));
    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buff, host_buff_, device_buff_, 1, &copy_region);
    COV_CHECK_ASSERT(vkEndCommandBuffer(cmd_buff));

    // submmit
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buff;

    VkFenceCreateInfo fence_info{};
    VkFence fence{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;
    COV_CHECK_ASSERT(vkCreateFence(device_, &fence_info, nullptr, &fence))
    COV_CHECK_ASSERT(vkQueueSubmit(queue_, 1, &submit_info, fence))
    COV_CHECK_ASSERT(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX))

    vkDestroyFence(device_, fence, nullptr);
    vkFreeCommandBuffers(device_, cmd_pool_, 1, &cmd_buff);
    return true;
}

bool Instance::to_host(void* data, size_t size)
{
    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.commandPool = cmd_pool_;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buff;
    COV_CHECK_ASSERT(vkAllocateCommandBuffers(device_, &cmd_alloc_info, &cmd_buff));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    COV_CHECK_ASSERT(vkBeginCommandBuffer(cmd_buff, &begin_info))
    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buff, device_buff_, host_buff_, 1, &copy_region);
    COV_CHECK_ASSERT(vkEndCommandBuffer(cmd_buff))

    // submit
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buff;

    VkFenceCreateInfo fence_info{};
    VkFence fence{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;
    COV_CHECK_ASSERT(vkCreateFence(device_, &fence_info, nullptr, &fence))
    COV_CHECK_ASSERT(vkQueueSubmit(queue_, 1, &submit_info, fence))
    COV_CHECK_ASSERT(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX))

    vkDestroyFence(device_, fence, nullptr);
    vkFreeCommandBuffers(device_, cmd_pool_, 1, &cmd_buff);

    void* mapped_data;
    vkMapMemory(device_, host_memory_, 0, size, 0, &mapped_data);
    memcpy(data, mapped_data, size);
    vkUnmapMemory(device_, host_memory_);
    return true;
}

bool Instance::load_shader(const std::string& shader_path, void* spec_data, size_t spec_data_len)
{
    std::ifstream ifs(shader_path, std::ios::in | std::ios::binary);
    if (ifs.is_open()) {
        ifs.seekg(0, std::ios::end);
        const auto size{ifs.tellg()};
        char* data{new char[size]};
        ifs.seekg(0, std::ios::beg);
        ifs.read(data, size);

        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = size;
        create_info.pCode = reinterpret_cast<uint32_t*>(data);
        COV_CHECK_ASSERT(vkCreateShaderModule(device_, &create_info, nullptr, &shader_module_))
        // specialization the shader
        spec_info_.dataSize = spec_data_len;
        spec_info_.pData = spec_data;

        delete [] data;
    } else {
        return false;
    }

    return true;
}

void Instance::add_spec_item(uint32_t const_id, uint32_t offset, size_t item_size)
{
    assert(spec_info_.pData != nullptr && spec_info_.dataSize > 0 &&
        "Specialization data is empty, no item to be add");

    VkSpecializationMapEntry spec_map_entry{};
    spec_map_entry.constantID = const_id;
    spec_map_entry.offset = offset;
    spec_map_entry.size = item_size;

    spec_map_entryies_.push_back(spec_map_entry);
    spec_info_.mapEntryCount = spec_map_entryies_.size();
    spec_info_.pMapEntries = spec_map_entryies_.data();
}

bool Instance::execute(int dims[3])
{
    VkDescriptorPool desc_pool{};
    VkPipelineLayout pipeline_layout{};
    std::vector<VkDescriptorSetLayout> desc_set_layout(3);
    std::vector<VkDescriptorSet> desc_set(3);
    VkPipelineCache pipeline_cache{};
    VkPipeline comp_pipeline{};

    if (shader_module_ == nullptr) {
        return false;
    }

    std::vector<VkDescriptorPoolSize> pool_sizes{
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 3}
    };
    VkDescriptorPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_create_info.pPoolSizes = pool_sizes.data();
    pool_create_info.maxSets = 3;
    COV_CHECK_ASSERT(vkCreateDescriptorPool(device_, &pool_create_info, nullptr, &desc_pool))

    std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 7,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        }
    };

    VkDescriptorSetLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
    layout_create_info.pBindings = set_layout_bindings.data();
    COV_CHECK_ASSERT(vkCreateDescriptorSetLayout(device_, &layout_create_info, nullptr, &desc_set_layout[0]))
    COV_CHECK_ASSERT(vkCreateDescriptorSetLayout(device_, &layout_create_info, nullptr, &desc_set_layout[1]))
    COV_CHECK_ASSERT(vkCreateDescriptorSetLayout(device_, &layout_create_info, nullptr, &desc_set_layout[2]))

    VkPipelineLayoutCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_create_info.setLayoutCount = desc_set_layout.size();
    pipeline_create_info.pSetLayouts = desc_set_layout.data();
    COV_CHECK_ASSERT(vkCreatePipelineLayout(device_, &pipeline_create_info, nullptr, &pipeline_layout))

    VkDescriptorSetAllocateInfo desc_alloc_info{};
    desc_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_alloc_info.descriptorSetCount = 3;
    desc_alloc_info.descriptorPool = desc_pool;
    desc_alloc_info.pSetLayouts = desc_set_layout.data();
    COV_CHECK_ASSERT(vkAllocateDescriptorSets(device_, &desc_alloc_info, desc_set.data()))

    std::vector<VkDescriptorBufferInfo> desc_buff_info(3);
    desc_buff_info[0].range = VK_WHOLE_SIZE;
    desc_buff_info[0].offset = 0;
    desc_buff_info[0].buffer = device_buff_;
    desc_buff_info[1].range = VK_WHOLE_SIZE;
    desc_buff_info[1].offset = 16;
    desc_buff_info[1].buffer = device_buff_;
    desc_buff_info[2].range = VK_WHOLE_SIZE;
    desc_buff_info[2].offset = 32;
    desc_buff_info[2].buffer = device_buff_;

    std::vector<VkWriteDescriptorSet> write_desc_set{
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = desc_set[0],
            .dstBinding = 7,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &desc_buff_info[0]
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = desc_set[1],
            .dstBinding = 7,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &desc_buff_info[1]
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = desc_set[2],
            .dstBinding = 7,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &desc_buff_info[2]
        }
    };
    vkUpdateDescriptorSets(device_, write_desc_set.size(), write_desc_set.data(), 0, nullptr);

    VkPipelineCacheCreateInfo pipeline_cache_create_info{};
    pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    COV_CHECK_ASSERT(vkCreatePipelineCache(device_, &pipeline_cache_create_info, nullptr, &pipeline_cache))

    VkPipelineShaderStageCreateInfo shader_stage_create_info{};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.module = shader_module_;
    if (spec_info_.mapEntryCount > 0) {
        shader_stage_create_info.pSpecializationInfo = &spec_info_;
    } else {
        shader_stage_create_info.pSpecializationInfo = VK_NULL_HANDLE;
    }
    shader_stage_create_info.pName = "main";
    shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    VkComputePipelineCreateInfo comp_pipeline_create_info{};
    comp_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    comp_pipeline_create_info.stage = shader_stage_create_info;
    comp_pipeline_create_info.layout = pipeline_layout;
    comp_pipeline_create_info.flags = 0;
    COV_CHECK_ASSERT(vkCreateComputePipelines(device_, pipeline_cache, 1, &comp_pipeline_create_info, nullptr, &comp_pipeline))

    VkCommandBuffer cmd_buff;
    VkCommandBufferAllocateInfo cmd_buff_alloc_info{};
    cmd_buff_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buff_alloc_info.commandBufferCount = 1;
    cmd_buff_alloc_info.commandPool = cmd_pool_;
    cmd_buff_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    COV_CHECK_ASSERT(vkAllocateCommandBuffers(device_, &cmd_buff_alloc_info, &cmd_buff))

    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    COV_CHECK_ASSERT(vkCreateFence(device_, &fence_create_info, nullptr, &fence))

    VkCommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    COV_CHECK_ASSERT(vkBeginCommandBuffer(cmd_buff, &cmd_begin_info))

    VkBufferMemoryBarrier mem_buff_barrier{};
    mem_buff_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    mem_buff_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_buff_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_buff_barrier.buffer = device_buff_;
    mem_buff_barrier.size = VK_WHOLE_SIZE;
    mem_buff_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    mem_buff_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd_buff, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
        0, nullptr,
        1, &mem_buff_barrier,
        0, nullptr);

    vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, comp_pipeline);
    vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 3, desc_set.data(), 0, nullptr);
    vkCmdDispatch(cmd_buff, dims[0], dims[1], dims[2]);

    // mem_buff_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // mem_buff_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // mem_buff_barrier.buffer = device_buff;
    // mem_buff_barrier.size = VK_WHOLE_SIZE;
    // mem_buff_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    // mem_buff_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    // vkCmdPipelineBarrier(cmd_buff, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
    //     0, nullptr,
    //     1, &mem_buff_barrier,
    //     0, nullptr);
    COV_CHECK_ASSERT(vkEndCommandBuffer(cmd_buff))
    vkResetFences(device_, 1, &fence);


    const VkPipelineStageFlags wait_stage_mask{VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buff;
    submit_info.pWaitDstStageMask = &wait_stage_mask;
    COV_CHECK_ASSERT(vkQueueSubmit(queue_, 1, &submit_info, fence))
    COV_CHECK_ASSERT(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX))


    vkDestroyPipelineCache(device_, pipeline_cache, nullptr);
    vkDestroyFence(device_, fence, nullptr);
    vkDestroyPipelineLayout(device_, pipeline_layout, nullptr);
    vkDestroyPipeline(device_, comp_pipeline, nullptr);
    vkDestroyDescriptorPool(device_, desc_pool, nullptr);
    for (auto dsl : desc_set_layout) {
        vkDestroyDescriptorSetLayout(device_, dsl, nullptr);
    }

    return true;
}

bool Instance::init_command_pool(VkDevice device, uint32_t queue_index, VkCommandPool& cmd_pool)
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

LayerExtensions::LayerExtensions()
{
    uint32_t count;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    exts_.resize(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, exts_.data());

    vkEnumerateInstanceLayerProperties(&count, nullptr);
    layers_.resize(count);
    vkEnumerateInstanceLayerProperties(&count, layers_.data());
}

const std::vector<VkExtensionProperties>& LayerExtensions::get_exts()
{
    return instance()->exts_;
}

const std::vector<VkLayerProperties>& LayerExtensions::get_layers()
{
    return instance()->layers_;
}

bool LayerExtensions::has_ext(const char* ext_name)
{
    const auto& exts{get_exts()};
    for (const auto& ext: exts) {
        std::cout << ext.extensionName << "\n";
        if (std::strcmp(ext_name, ext.extensionName) == 0) {
            return true;
        }
    }
    return false;
}

bool LayerExtensions::has_layer(const char* layer_name)
{
    const auto& layers{get_layers()};
    for (const auto& layer: layers) {
        std::cout << layer.layerName << "\n";
        if (std::strcmp(layer_name, layer.layerName) == 0) {
            return true;
        }
    }
    return false;
}

PhysicalDevice::PhysicalDevice() {}

bool PhysicalDevice::get(const VkInstance& vk_ins, VkPhysicalDevice& device, uint32_t& que_family_index)
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

bool PhysicalDevice::property_available(VkPhysicalDevice device)
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

std::optional<uint32_t> PhysicalDevice::find_available_queue(VkPhysicalDevice device)
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

void PhysicalDevice::properties(VkPhysicalDevice device, VkPhysicalDeviceProperties& properties)
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
#if COV_ENABLE_VALIDATION
    CHECK_VALIDATION_AVAILABLE();
    create_info.ppEnabledLayerNames = cov_validation_layers.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(cov_validation_layers.size());
#else // COV_ENABLE_VALIDATION
    create_info.enabledLayerCount = 0;
#endif // COV_ENABLE_VALIDATION

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

bool create_buffer(VkDevice device, VkPhysicalDevice phy_device, size_t size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags property_flags, VkBuffer& buff, VkDeviceMemory& memory)
{
    // create buffer
    VkBufferCreateInfo buff_create_info{};
    buff_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_create_info.size = size;
    buff_create_info.usage = usage;
    buff_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    COV_CHECK_ASSERT(vkCreateBuffer(device, &buff_create_info, nullptr, &buff))

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

    COV_CHECK_ASSERT(vkAllocateMemory(device, &mem_alloc_info, nullptr, &memory))
    COV_CHECK_ASSERT(vkBindBufferMemory(device, buff, memory, 0))
    return true;
}

std::string stringify(VkResult result)
{
#define COV_MATCH_STRINGIFY(r) case VK_##r: return #r;

    switch (result) {
    COV_MATCH_STRINGIFY(SUCCESS)
    COV_MATCH_STRINGIFY(NOT_READY)
    COV_MATCH_STRINGIFY(TIMEOUT)
    COV_MATCH_STRINGIFY(EVENT_SET)
    COV_MATCH_STRINGIFY(EVENT_RESET)
    COV_MATCH_STRINGIFY(INCOMPLETE)
    COV_MATCH_STRINGIFY(ERROR_OUT_OF_HOST_MEMORY)
    COV_MATCH_STRINGIFY(ERROR_OUT_OF_DEVICE_MEMORY)
    COV_MATCH_STRINGIFY(ERROR_INITIALIZATION_FAILED)
    COV_MATCH_STRINGIFY(ERROR_DEVICE_LOST)
    COV_MATCH_STRINGIFY(ERROR_MEMORY_MAP_FAILED)
    COV_MATCH_STRINGIFY(ERROR_LAYER_NOT_PRESENT)
    COV_MATCH_STRINGIFY(ERROR_EXTENSION_NOT_PRESENT)
    COV_MATCH_STRINGIFY(ERROR_FEATURE_NOT_PRESENT)
    COV_MATCH_STRINGIFY(ERROR_INCOMPATIBLE_DRIVER)
    COV_MATCH_STRINGIFY(ERROR_TOO_MANY_OBJECTS)
    COV_MATCH_STRINGIFY(ERROR_FORMAT_NOT_SUPPORTED)
    COV_MATCH_STRINGIFY(ERROR_FRAGMENTED_POOL)
    COV_MATCH_STRINGIFY(ERROR_UNKNOWN)
    COV_MATCH_STRINGIFY(ERROR_OUT_OF_POOL_MEMORY)
    COV_MATCH_STRINGIFY(ERROR_INVALID_EXTERNAL_HANDLE)
    COV_MATCH_STRINGIFY(ERROR_FRAGMENTATION)
    COV_MATCH_STRINGIFY(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
    COV_MATCH_STRINGIFY(PIPELINE_COMPILE_REQUIRED)
    COV_MATCH_STRINGIFY(ERROR_NOT_PERMITTED)
    COV_MATCH_STRINGIFY(ERROR_SURFACE_LOST_KHR)
    COV_MATCH_STRINGIFY(ERROR_NATIVE_WINDOW_IN_USE_KHR)
    COV_MATCH_STRINGIFY(SUBOPTIMAL_KHR)
    COV_MATCH_STRINGIFY(ERROR_OUT_OF_DATE_KHR)
    COV_MATCH_STRINGIFY(ERROR_INCOMPATIBLE_DISPLAY_KHR)
    COV_MATCH_STRINGIFY(ERROR_VALIDATION_FAILED_EXT)
    COV_MATCH_STRINGIFY(ERROR_INVALID_SHADER_NV)
    COV_MATCH_STRINGIFY(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
    COV_MATCH_STRINGIFY(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
    COV_MATCH_STRINGIFY(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
    COV_MATCH_STRINGIFY(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
    COV_MATCH_STRINGIFY(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
    COV_MATCH_STRINGIFY(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
    COV_MATCH_STRINGIFY(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
    COV_MATCH_STRINGIFY(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
    COV_MATCH_STRINGIFY(THREAD_IDLE_KHR)
    COV_MATCH_STRINGIFY(THREAD_DONE_KHR)
    COV_MATCH_STRINGIFY(OPERATION_DEFERRED_KHR)
    COV_MATCH_STRINGIFY(OPERATION_NOT_DEFERRED_KHR)
    COV_MATCH_STRINGIFY(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
    COV_MATCH_STRINGIFY(ERROR_COMPRESSION_EXHAUSTED_EXT)
    COV_MATCH_STRINGIFY(INCOMPATIBLE_SHADER_BINARY_EXT)
    COV_MATCH_STRINGIFY(PIPELINE_BINARY_MISSING_KHR)
    COV_MATCH_STRINGIFY(ERROR_NOT_ENOUGH_SPACE_KHR)
    default: return "UNKNOWN ERROR";
    }
#undef COV_MATCH_STRINGIFY
}

} // namespace cov

#endif // COV_IMPLEMENTATION_CPP_
#endif // COV_IMPLEMENTATION
