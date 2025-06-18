#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#define CVK_IMPLEMENTATION
#include "cvk.hpp"


int main()
{
    VkInstance vk_instance;

    cvk::App::init("Hello");
    if (!cvk::App::create_instance(vk_instance)) {
        std::cerr << "Failed to create vk instance\n";
        return 1;
    }

    const auto& exts{cvk::Extensions::get()};
    for (auto& ext: exts) {
        std::cout << ext.extensionName << ": " << ext.specVersion << "\n";
    }

    VkPhysicalDevice phy_device;
    uint32_t queue_index;
    if (!cvk::PhyDevice::get(vk_instance, phy_device, queue_index)) {
        std::cerr << "No available physical device\n";
        return 1;
    }

    VkPhysicalDeviceProperties properties;
    cvk::PhyDevice::properties(phy_device, properties);
    std::cout << "GPU Device: " << properties.deviceName << "\n";

    VkDevice device;
    VkQueue queue;
    if (!cvk::Device::create(phy_device, queue_index, device, queue)) {
        std::cerr << "Failed to create device\n";
        return 1;
    }

    VkCommandPool cmd_pool;
    if (!cvk::CommandPool::create(device, queue_index, cmd_pool)) {
        std::cerr << "Failed to create command pool";
        return 1;
    }

    std::vector<int> data{1, 2, 3, 4, 5, 6};

    cvk::to_device(data.data(), data.size() * sizeof(data.at(0)), device, phy_device, cmd_pool, queue);

    cvk::Device::destroy(device);
    cvk::App::destroy_all_instance();
    return 0;
}