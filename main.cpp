#include <iostream>
#include <vector>

#define CVK_IMPLEMENTATION
#include "cvk.hpp"


int main()
{
    VkInstance vk_instance;

    cvk::VkIns::init("Hello");
    if (!cvk::VkIns::create(vk_instance)) {
        std::cerr << "Failed to create vk instance\n";
        return 1;
    }

    const auto& exts{cvk::VkExts::get()};
    for (auto& ext: exts) {
        std::cout << ext.extensionName << ": " << ext.specVersion << "\n";
    }

    cvk::VkIns::destroy_all();
    return 0;
}