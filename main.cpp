#include <iostream>

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

    auto phy_device{cvk::PhyDevice::get(vk_instance)};
    if (phy_device == nullptr) {
        std::cerr << "No available physical device\n";
        return 1;
    }

    cvk::App::destroy_all_instance();
    return 0;
}