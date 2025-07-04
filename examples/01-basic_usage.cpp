#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#define CVK_IMPLEMENTATION
#include "cov.hpp"


struct SpecializationData {
    uint32_t num_element;
}; // struct SpecializationData


int main()
{
    const std::string shader_path{"../examples/shader/headless.comp.spv"};  // suppose we run this program on build dir

    std::vector<int> in_data{1, 2, 3, 4, 5, 6, 7};
    std::vector<int> out_data(in_data.size());
    
    cov::App::init("HelloCVK");

    {
        SpecializationData spec_data{.num_element = 32};
        // core code
        auto instance{cov::App::new_instance()};
        instance.load_shader(shader_path, &spec_data, sizeof(spec_data));
        instance.add_spec_item(0, 0, sizeof(uint32_t));

        instance.to_device(in_data.data(), in_data.size() * sizeof(in_data.at(0)));
        if (!instance.execute()) {
            std::cerr << "execute shader program failed\n";
        }
        instance.to_host(out_data.data(), out_data.size() * sizeof(out_data.at(0)));
        // The instance will be automatically destroy here.
    }

    for (auto it: out_data) {
        std::cout << it << "\n";
    }
    return 0;
}
