#include <algorithm>
#include <iostream>
#include <vector>

#define CVK_IMPLEMENTATION
#include "cvk.hpp"


int main()
{
    const std::string shader_path{"../examples/shader/headless.comp.spv"};  // suppose we run this program on build dir

    std::vector<int> in_data{1, 2, 3, 4, 5, 6, 7};
    std::vector<int> out_data(in_data.size());
    
    cvk::App::init("HelloCVK");

    {
        // core code
        auto instance{cvk::App::new_instance()};
        instance.load_shader(shader_path);
        auto move_instance = std::move(instance);   // move test
        move_instance.to_device(in_data.data(), in_data.size() * sizeof(in_data.at(0)));
        move_instance.execute();
        move_instance.to_host(out_data.data(), out_data.size() * sizeof(out_data.at(0)));
        // The instance will be automatically destroy here.
    }

    for (auto it: out_data) {
        std::cout << it << "\n";
    }
    return 0;
}
