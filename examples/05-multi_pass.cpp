#include <cstdint>
#include <iostream>
#include <vector>
#include <string>

#define COV_VULKAN_VALIDATION
#define COV_IMPLEMENTATION
#include "cov.hpp"


int main()
{
    const std::string shader_pass1{"../examples/shader/headless.comp.spv"};  // suppose we run this program on build dir
    const std::string shader_pass2{"../examples/shader/headless.comp.spv"};  // suppose we run this program on build dir
    std::vector<uint8_t> data;

    // execute once
    cov::App::init("HelloMlutiPass");
    auto instance{cov::App::new_instance()};
    int mapping_id0{instance.add_mem_mapping(100)};
    int mapping_id1{instance.add_mem_mapping(data.size())};

    instance.add_pass(shader_pass1, {mapping_id0}, {4, 1, 1});
        // .add_spec_item(const_id, offset, item_size, spec_data, spec_data_len);
    instance.add_pass(shader_pass2, {mapping_id0}, {4, 1, 1});
        // .add_spec_item(const_id, offset, item_size, spec_data, spec_data_len);

    {
        // execute multi-times
        instance.to_device(mapping_id0, data.data(), data.size());
        if (!instance.execute()) {
            std::cerr << "Execute shader program failed\n";
        }
        instance.from_device(mapping_id1, data.data(), data.size());
    }
    // The instance will be automatically destroy here.

    return 0;
}
