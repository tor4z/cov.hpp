#include <iostream>
#include <vector>

#define CVK_IMPLEMENTATION
#include "cvk.hpp"


int main()
{
    const std::string shader_path{"../headless.comp.spv"};

    std::vector<int> in_data{1, 2, 3, 4, 5, 6, 7};
    std::vector<int> out_data(in_data.size());
    
    cvk::App::init("HelloCVK");
    auto instance{cvk::App::new_instance()};
    instance.to_device(in_data.data(), in_data.size() * sizeof(in_data.at(0)));
    instance.execute(shader_path);
    instance.to_host(out_data.data(), out_data.size() * sizeof(out_data.at(0)));

    for (auto it: out_data) {
        std::cout << it << "\n";
    }
    // Instance will be automatically destroy instance here.
    return 0;
}
