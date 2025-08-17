#ifndef COV_H_
#define COV_H_

#include <array>
#include <mutex>
#include <optional>
#include <vector>
#include <string_view>
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

class Instance;

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

struct MemMapping
{
    enum AccessStage {
        AS_UNKNOWN = 0,
        AS_TRANSFER_R,
        AS_TRANSFER_W,
        AS_COMPUTE_R,
        AS_COMPUTE_W,
    };

    MemMapping(const MemMapping& other);
    MemMapping(MemMapping&& other);
    MemMapping& operator=(const MemMapping& other);
    MemMapping& operator=(MemMapping&& other);
    bool copy_from(const void* ptr, size_t size);
    bool copy_to(void* ptr, size_t size);
private:
    friend struct TransferPass;
    friend struct ComputePass;
    friend class Instance;

    explicit MemMapping(Instance* instance)
        : instance(instance)
        , host_buff(VK_NULL_HANDLE)
        , host_memory(VK_NULL_HANDLE)
        , device_buff(VK_NULL_HANDLE)
        , device_memory(VK_NULL_HANDLE)
        , size(0)
        , stage(AS_UNKNOWN) {}
    void destroy();

    Instance* instance;
    VkBuffer host_buff;
    VkDeviceMemory host_memory;
    VkBuffer device_buff;
    VkDeviceMemory device_memory;
    size_t size;
    AccessStage stage;
}; // struct MemMapping

struct TransferPass
{
    TransferPass* to_device(MemMapping* mapping);
    TransferPass* from_device(MemMapping* mapping);
    bool build();
    bool destroy() { return true; }
private:
    friend class MemMapping;
    friend class Instance;
    Instance* instance;

    explicit TransferPass(Instance* instance) : instance(instance) {}
}; // TransferPass

struct ComputePass
{
    ComputePass* set_inputs(const std::vector<MemMapping*>& input_mappings);
    ComputePass* set_outputs(const std::vector<MemMapping*>& output_mappings);
    ComputePass* set_workgroup_dims(int x, int y, int z);
    ComputePass* load_shader_from_file(const std::string_view& shader_path);
    ComputePass* load_bin_shader(const void* shader, size_t size);
    bool build();
private:
    friend class Instance;
    friend class MemMapping;
    explicit ComputePass(Instance* instance);
    void destroy(VkDevice device);
    bool build_comp_pipeline();
    bool build_descriptor_set();

    Instance* instance;
    std::vector<MemMapping*> used_mappings;
    std::vector<VkDescriptorSet> desc_set;
    std::vector<VkDescriptorSetLayout> desc_set_layout;
    std::vector<VkBufferMemoryBarrier> mem_buf_barriers;
    std::array<int, 3> workgroup_dims;
    VkPipeline comp_pipeline;
    VkDescriptorPool desc_pool;
    VkShaderModule shader_module;
    VkPipelineLayout pipeline_layout;
    VkPipelineCache pipeline_cache;
}; // struct ComputePass

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

    MemMapping* add_mem_mapping(size_t size);
    ComputePass* add_compute_pass();
    TransferPass* add_transfer_pass();
    bool execute();
    void destroy();
private:
    friend struct TransferPass;
    friend struct ComputePass;
    friend struct MemMapping;

    enum CmdBufStatus {
        CBS_UNKNOWN = 0,
        CBS_BEGAN,
        CBS_ENDED,
    }; // enum CmdBufStatus

    VkInstance vk_instance_;
    VkCommandPool cmd_pool_;
    VkQueue queue_;
    VkDevice device_;
    VkCommandBuffer cmd_buf_;
    VkPhysicalDevice phy_device_;
    VkSpecializationInfo spec_info_;
    std::vector<ComputePass*> comp_passes_;
    std::vector<TransferPass*> transfer_passes_;
    std::vector<MemMapping*> mem_mappings_;
    std::vector<VkSpecializationMapEntry> spec_map_entryies_;
    uint32_t queue_index_;
    CmdBufStatus cmd_buf_status_;

    friend class Vulkan;
    Instance(VkInstance vk_instance);
    void create_cmd_buf();
    void try_begin_cmd_buf();
    void try_end_cmd_buf();
    static bool init_command_pool(VkDevice device, uint32_t queue_index, VkCommandPool& cmd_pool);
}; // class Instance

class Vulkan
{
public:
    static void init(const char* app_name);
    static Instance new_instance();
private:
    VkApplicationInfo vk_app_info_;

    COV_DEF_SINGLETON(Vulkan)
    Vulkan() = default;
    ~Vulkan() = default;
}; // class Vulkan

} // namespace cov

#endif // COV_H_

// =======================================
//            Implementation
// =======================================
#define COV_IMPLEMENTATION // please delete me


#ifdef COV_IMPLEMENTATION
#ifndef COV_IMPLEMENTATION_CPP_
#define COV_IMPLEMENTATION_CPP_

#include <ios>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cassert>
#include <cstddef>
#include <fstream>
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

void Vulkan::init(const char* app_name)
{
    auto ins{instance()};
    ins->vk_app_info_.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ins->vk_app_info_.pApplicationName = app_name;
    ins->vk_app_info_.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ins->vk_app_info_.pEngineName = "No Engine";
    ins->vk_app_info_.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ins->vk_app_info_.apiVersion = VK_API_VERSION_1_4;
}

Instance Vulkan::new_instance()
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
    , queue_(VK_NULL_HANDLE)
    , phy_device_(VK_NULL_HANDLE)
    , device_(VK_NULL_HANDLE)
    , cmd_buf_(VK_NULL_HANDLE)
    , queue_index_(-1)
    , cmd_buf_status_(CBS_UNKNOWN)
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
    mem_mappings_ = other.mem_mappings_;
    queue_ = other.queue_;
    phy_device_ = other.phy_device_;
    device_ = other.device_;
    queue_index_ = other.queue_index_;
    spec_info_ = other.spec_info_;
    spec_map_entryies_ = other.spec_map_entryies_;
    cmd_buf_status_ = other.cmd_buf_status_;

    other.vk_instance_ = VK_NULL_HANDLE;
    other.cmd_pool_ = VK_NULL_HANDLE;
    other.queue_ = VK_NULL_HANDLE;
    other.phy_device_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.queue_index_ = -1;
    other.mem_mappings_.clear();
    other.spec_map_entryies_.clear();
    cmd_buf_status_ = CBS_UNKNOWN;
}

Instance& Instance::operator=(Instance&& other)
{
    if (this == &other) {
        return *this;
    }

    destroy();
    vk_instance_ = other.vk_instance_;
    cmd_pool_ = other.cmd_pool_;
    mem_mappings_ = other.mem_mappings_;
    queue_ = other.queue_;
    phy_device_ = other.phy_device_;
    device_ = other.device_;
    queue_index_ = other.queue_index_;
    spec_info_ = other.spec_info_;
    spec_map_entryies_ = other.spec_map_entryies_;
    cmd_buf_status_ = other.cmd_buf_status_;

    other.vk_instance_ = VK_NULL_HANDLE;
    other.cmd_pool_ = VK_NULL_HANDLE;
    other.queue_ = VK_NULL_HANDLE;
    other.phy_device_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.queue_index_ = -1;
    other.mem_mappings_.clear();
    other.spec_map_entryies_.clear();
    cmd_buf_status_ = CBS_UNKNOWN;

    return *this;
}

void Instance::destroy()
{
    if (device_ && cmd_pool_) {
        vkDestroyCommandPool(device_, cmd_pool_, nullptr);
    }

    for (const auto& mapping : mem_mappings_) {
        mapping->destroy();
        delete mapping;
    }
    mem_mappings_.clear();

    for (auto& pass : comp_passes_) {
        pass->destroy(device_);
        delete pass;
    }
    comp_passes_.clear();

    for (auto& pass : transfer_passes_) {
        pass->destroy();
        delete pass;
    }
    transfer_passes_.clear();

    if (device_) {
        vkDestroyDevice(device_, nullptr);
    }
    if (vk_instance_) {
        vkDestroyInstance(vk_instance_, nullptr);
    }

    spec_map_entryies_.clear();
}

MemMapping* Instance::add_mem_mapping(size_t size)
{
    assert(size > 0 && "Bad buffer size");

    mem_mappings_.push_back(new MemMapping{this});
    auto mapping{mem_mappings_.back()};

    mapping->size = size;
    create_buffer(device_, phy_device_, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, mapping->host_buff, mapping->host_memory);
    create_buffer(device_, phy_device_, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mapping->device_buff, mapping->device_memory);

    return mapping;
}

void Instance::create_cmd_buf()
{
    if (cmd_buf_ != VK_NULL_HANDLE) {
        return;
    }
    VkCommandBufferAllocateInfo cmd_buf_alloc_info{};
    cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_alloc_info.commandBufferCount = 1;
    cmd_buf_alloc_info.commandPool = cmd_pool_;
    cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    COV_CHECK_ASSERT(vkAllocateCommandBuffers(device_, &cmd_buf_alloc_info, &cmd_buf_))
}

void Instance::try_begin_cmd_buf()
{
    if (cmd_buf_status_ == CBS_UNKNOWN) {
        create_cmd_buf();
        VkCommandBufferBeginInfo cmd_begin_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        COV_CHECK_ASSERT(vkBeginCommandBuffer(cmd_buf_, &cmd_begin_info));
        cmd_buf_status_ = CBS_BEGAN;
    }
}

void Instance::try_end_cmd_buf()
{
    if (cmd_buf_status_ == CBS_BEGAN) {
        COV_CHECK_ASSERT(vkEndCommandBuffer(cmd_buf_));    
        cmd_buf_status_ = CBS_ENDED;
    }
}

bool Instance::execute()
{
    try_end_cmd_buf();

    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    COV_CHECK_ASSERT(vkCreateFence(device_, &fence_create_info, nullptr, &fence))

    vkResetFences(device_, 1, &fence);

    const VkPipelineStageFlags wait_stage_mask{VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf_;
    submit_info.pWaitDstStageMask = &wait_stage_mask;
    COV_CHECK_ASSERT(vkQueueSubmit(queue_, 1, &submit_info, fence))
    COV_CHECK_ASSERT(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX))

    vkDestroyFence(device_, fence, nullptr);
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

TransferPass* Instance::add_transfer_pass()
{
    transfer_passes_.push_back(new TransferPass{this});
    auto pass{transfer_passes_.back()};
    return pass;
}

ComputePass* Instance::add_compute_pass()
{
    comp_passes_.push_back(new ComputePass{this});
    auto pass{comp_passes_.back()};
    return pass;
}

void MemMapping::destroy()
{
    vkDestroyBuffer(instance->device_, device_buff, nullptr);
    vkDestroyBuffer(instance->device_, host_buff, nullptr);
    vkFreeMemory(instance->device_, device_memory, nullptr);
    vkFreeMemory(instance->device_, host_memory, nullptr);
}

MemMapping::MemMapping(const MemMapping& other)
{
    instance = other.instance;
    host_buff = other.host_buff;
    host_memory = other.host_memory;
    device_buff = other.device_buff;
    device_memory = other.device_memory;
    size = other.size;
}

MemMapping::MemMapping(MemMapping&& other)
{
    instance = other.instance;
    host_buff = other.host_buff;
    host_memory = other.host_memory;
    device_buff = other.device_buff;
    device_memory = other.device_memory;
    size = other.size;

    instance = nullptr;
    host_buff = VK_NULL_HANDLE;
    host_memory = VK_NULL_HANDLE;
    device_buff = VK_NULL_HANDLE;
    device_memory = VK_NULL_HANDLE;
    size = 0;
}

MemMapping& MemMapping::operator=(const MemMapping& other)
{
    instance = other.instance;
    host_buff = other.host_buff;
    host_memory = other.host_memory;
    device_buff = other.device_buff;
    device_memory = other.device_memory;
    size = other.size;
    return *this;
}

MemMapping& MemMapping::operator=(MemMapping&& other)
{
    instance = nullptr;
    host_buff = VK_NULL_HANDLE;
    host_memory = VK_NULL_HANDLE;
    device_buff = VK_NULL_HANDLE;
    device_memory = VK_NULL_HANDLE;
    size = 0;
    return *this;
}

bool MemMapping::copy_from(const void* ptr, size_t size)
{
    assert(ptr != nullptr && "Invalid pointer");
    assert(size > 0 && "Invalid buffer size");
    assert(size <= this->size && "Invalid buffer size greate than pre-allocated buffer size");

    void* mapped_data;
    VkMappedMemoryRange mem_range{.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
    vkMapMemory(instance->device_, host_memory, 0, size, 0, &mapped_data);
    memcpy(mapped_data, ptr, size);
    vkUnmapMemory(instance->device_, host_memory);
    return true;
}

bool MemMapping::copy_to(void* ptr, size_t size)
{
    assert(ptr != nullptr && "Invalid pointer");
    assert(size > 0 && "Invalid buffer size");
    assert(size <= this->size && "Invalid buffer size greate than pre-allocated buffer size");

    void* mapped_data;
    vkMapMemory(instance->device_, host_memory, 0, size, 0, &mapped_data);
    memcpy(ptr, mapped_data, size);
    vkUnmapMemory(instance->device_, host_memory);
    return true;
}

TransferPass* TransferPass::to_device(MemMapping* mapping)
{
    assert(mapping != nullptr && "Invalid memory mapping");

    instance->try_begin_cmd_buf();
    mapping->stage = MemMapping::AS_TRANSFER_W;
    VkBufferCopy copy_region{.size = mapping->size};
    vkCmdCopyBuffer(instance->cmd_buf_, mapping->host_buff, mapping->device_buff, 1, &copy_region);
    return this;
}

TransferPass* TransferPass::from_device(MemMapping* mapping)
{
    assert(mapping != nullptr && "Invalid memory mapping");

    instance->try_begin_cmd_buf();
    mapping->stage = MemMapping::AS_TRANSFER_R;
    VkBufferCopy copy_region{.size = mapping->size};
    vkCmdCopyBuffer(instance->cmd_buf_, mapping->device_buff, mapping->host_buff, 1, &copy_region);
    return this;
}

bool TransferPass::build()
{
    return true;
}

ComputePass::ComputePass(Instance* instance)
    : instance(instance)
    , workgroup_dims({1, 1, 1})
{
}

ComputePass* ComputePass::load_shader_from_file(const std::string_view& shader_path)
{
    std::ifstream ifs(shader_path.data(), std::ios::in | std::ios::binary);
    if (ifs.is_open()) {
        ifs.seekg(0, std::ios::end);
        const auto size{ifs.tellg()};
        char* data{new char[size]};
        ifs.seekg(0, std::ios::beg);
        ifs.read(data, size);
        load_bin_shader(data, size);
        delete [] data;
    } else {
        assert(false && "Load shader fro file failed");
    }

    return this;
}

ComputePass* ComputePass::load_bin_shader(const void* shader, size_t size)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = size;
    create_info.pCode = reinterpret_cast<const uint32_t*>(shader);
    COV_CHECK_ASSERT(vkCreateShaderModule(instance->device_, &create_info, nullptr, &shader_module))
    return this;
}

bool ComputePass::build_comp_pipeline()
{
    VkPipelineLayoutCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_create_info.setLayoutCount = desc_set_layout.size();
    pipeline_create_info.pSetLayouts = desc_set_layout.data();
    COV_CHECK_ASSERT(vkCreatePipelineLayout(instance->device_, &pipeline_create_info, nullptr, &pipeline_layout))

    VkPipelineCacheCreateInfo pipeline_cache_create_info{};
    pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    COV_CHECK_ASSERT(vkCreatePipelineCache(instance->device_, &pipeline_cache_create_info, nullptr, &pipeline_cache))

    VkPipelineShaderStageCreateInfo shader_stage_create_info{};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.module = shader_module;
    if (instance->spec_info_.mapEntryCount > 0) {
        shader_stage_create_info.pSpecializationInfo = &instance->spec_info_;
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
    COV_CHECK_ASSERT(vkCreateComputePipelines(instance->device_, pipeline_cache, 1, &comp_pipeline_create_info, nullptr, &comp_pipeline))
    return true;
}

ComputePass* ComputePass::set_outputs(const std::vector<MemMapping*>& output_mappings)
{
    used_mappings.insert(used_mappings.end(), output_mappings.begin(), output_mappings.end());
    for (auto mapping : output_mappings) {
        mapping->stage = MemMapping::AS_COMPUTE_W;
    }
    return this;
}

ComputePass* ComputePass::set_inputs(const std::vector<MemMapping*>& input_mappings)
{
    mem_buf_barriers.resize(input_mappings.size());
    for (size_t i = 0; i < input_mappings.size(); ++i) {
        auto& mapping{input_mappings.at(i)};
        if (mapping->stage != MemMapping::AS_TRANSFER_W && mapping->stage != MemMapping::AS_COMPUTE_W) {
            continue;
        }

        auto& mem_barrier{mem_buf_barriers.at(i)};
        mem_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mem_barrier.buffer = mapping->device_buff;
        mem_barrier.size = VK_WHOLE_SIZE;
        if (mapping->stage == MemMapping::AS_TRANSFER_W) {
            mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        } else if (mapping->stage == MemMapping::AS_COMPUTE_W) {
            mem_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        }
        mem_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    used_mappings.insert(used_mappings.begin(), input_mappings.begin(), input_mappings.end());
    return this;
}


bool ComputePass::build_descriptor_set()
{
    desc_set.resize(used_mappings.size());
    desc_set_layout.resize(used_mappings.size());

    std::vector<VkDescriptorPoolSize> pool_sizes{
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = static_cast<uint32_t>(used_mappings.size())}
    };
    VkDescriptorPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_create_info.pPoolSizes = pool_sizes.data();
    pool_create_info.maxSets = used_mappings.size();
    COV_CHECK_ASSERT(vkCreateDescriptorPool(instance->device_, &pool_create_info, nullptr, &desc_pool))

    std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        }
    };

    VkDescriptorSetLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
    layout_create_info.pBindings = set_layout_bindings.data();
    for (size_t i = 0; i < used_mappings.size(); ++i) {
        COV_CHECK_ASSERT(vkCreateDescriptorSetLayout(instance->device_, &layout_create_info, nullptr, &desc_set_layout.at(i)))
    }

    VkDescriptorSetAllocateInfo desc_alloc_info{};
    desc_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_alloc_info.descriptorSetCount = used_mappings.size();
    desc_alloc_info.descriptorPool = desc_pool;
    desc_alloc_info.pSetLayouts = desc_set_layout.data();
    COV_CHECK_ASSERT(vkAllocateDescriptorSets(instance->device_, &desc_alloc_info, desc_set.data()))

    std::vector<VkDescriptorBufferInfo> desc_buff_info(used_mappings.size());
    for (size_t i = 0; i < used_mappings.size(); ++i) {
        const auto& mapping{used_mappings.at(i)};
        desc_buff_info[i].range = VK_WHOLE_SIZE;
        desc_buff_info[i].offset = 0;
        desc_buff_info[i].buffer = mapping->device_buff;
    }

    std::vector<VkWriteDescriptorSet> write_desc_sets;
    write_desc_sets.reserve(used_mappings.size());
    for (size_t i = 0; i < used_mappings.size(); ++i) {
        write_desc_sets.emplace_back(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = desc_set.at(i),
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &desc_buff_info.at(i)
        });
    }

    vkUpdateDescriptorSets(instance->device_, write_desc_sets.size(), write_desc_sets.data(), 0, nullptr);
    return this;
}

ComputePass* ComputePass::set_workgroup_dims(int x, int y, int z)
{
    workgroup_dims.at(0) = x;
    workgroup_dims.at(1) = y;
    workgroup_dims.at(2) = z;
    return this;
}

bool ComputePass::build()
{
    build_descriptor_set();
    build_comp_pipeline();
    instance->try_begin_cmd_buf();

    if (!mem_buf_barriers.empty()) {
        std::vector<VkBufferMemoryBarrier> transfer_stage_barriers;
        std::vector<VkBufferMemoryBarrier> compute_stage_barriers;

        for (auto& it : mem_buf_barriers) {
            if (it.srcAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT) {
                transfer_stage_barriers.push_back(it);
            } else {
                compute_stage_barriers.push_back(it);
            }
        }

        if (!transfer_stage_barriers.empty()) {
            vkCmdPipelineBarrier(instance->cmd_buf_, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                0, nullptr,
                transfer_stage_barriers.size(), transfer_stage_barriers.data(),
                0, nullptr);
        }
        if (!compute_stage_barriers.empty()) {
            vkCmdPipelineBarrier(instance->cmd_buf_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                0, nullptr,
                compute_stage_barriers.size(), compute_stage_barriers.data(),
                0, nullptr);
        }

    }
    vkCmdBindPipeline(instance->cmd_buf_, VK_PIPELINE_BIND_POINT_COMPUTE, comp_pipeline);
    vkCmdBindDescriptorSets(instance->cmd_buf_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout,
        0, desc_set.size(), desc_set.data(), 0, nullptr);
    vkCmdDispatch(instance->cmd_buf_, workgroup_dims.at(0), workgroup_dims.at(1), workgroup_dims.at(2));

    vkDestroyShaderModule(instance->device_, shader_module, nullptr);
    vkDestroyPipelineCache(instance->device_, pipeline_cache, nullptr);
    vkDestroyPipelineLayout(instance->device_, pipeline_layout, nullptr);
    for (auto dsl : desc_set_layout) {
        vkDestroyDescriptorSetLayout(instance->device_, dsl, nullptr);
    }
    return true;
}

void ComputePass::destroy(VkDevice device) {
    vkDestroyPipeline(device, comp_pipeline, nullptr);
    vkDestroyDescriptorPool(device, desc_pool, nullptr);
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
