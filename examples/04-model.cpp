#include <cstdint>
#include <cstring>

#define COV_VULKAN_VALIDATION
#define COV_IMPLEMENTATION
#include "cov.hpp"


int main()
{
    // // suppose we run this program on build dir
    // // const std::string shader_path{"../examples/shader/multi_layer_matmul.comp.spv"};
    // // const std::string shader_path{"../examples/shader/multi_layer_matmul2.comp.spv"};
    // const std::string shader_path{"../examples/shader/model.comp.spv"};

    // cov::App::init("BasicMlpModel");

    // {
    //     // core code
    //     auto instance{cov::App::new_instance()};
    //     instance.load_shader(shader_path);

    //     std::vector<char> input(4 + 6 * 4 + 4 * 4);
    //     uint32_t dims{2};
    //     uint32_t shape[6]{1, 4, 0, 0, 0, 0};
    //     memcpy(input.data(), &dims, sizeof(dims));
    //     memcpy(input.data() + 4, shape, 6 * sizeof(uint32_t));
    //     std::vector<char> output(4 + 6 * 4 + 4 * 4, 0);

    //     instance.set_inputs({
    //         {input.data(), input.size()},
    //     });
    //     instance.def_output(output.size() * sizeof(output.at(0)));

    //     if (!instance.execute({32, 32, 1})) {
    //         std::cerr << "Execute shader program failed\n";
    //     }
    //     instance.get_output(output.data(), output.size());
    //     // The instance will be automatically destroy here.

    //     std::cout << "Output:\n";
    //     std::cout << "dims: " << *reinterpret_cast<uint32_t*>(output.data()) << "\n";
    //     std::cout << "shape: ";
    //     for (int i = 0; i < 6; ++i) {
    //         std::cout << *reinterpret_cast<uint32_t*>(output.data() + 4 * (i + 1)) << " ";
    //     }
    //     std::cout << "\ndata: ";
    //     for (int i = 0; i < 4; ++i) {
    //         std::cout << *reinterpret_cast<float*>(output.data() + 4 * (i + 1 + 6)) << " ";
    //     }
    //     std::cout << "\n";
    // }

    return 0;
}
