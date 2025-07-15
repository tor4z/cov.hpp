#include <vector>

#define COV_VULKAN_VALIDATION
#define COV_IMPLEMENTATION
#include "cov.hpp"

struct Mat
{
    int dims;
    int col;
    int row;
    std::vector<float> data;
    void as_blob(std::vector);
}; // struct Mat




int main()
{
    const std::string shader_path{"../examples/shader/matmul.comp.spv"};  // suppose we run this program on build dir

    Mat A;
    Mat B;
    Mat C;
    cov::App::init("Matmul");

    {
        SpecializationData spec_data{.num_element = static_cast<uint32_t>(in_data.size())};
        // core code
        auto instance{cov::App::new_instance()};
        instance.load_shader(shader_path, &spec_data, sizeof(spec_data));
        instance.add_spec_item(0, 0, sizeof(uint32_t));
        instance.set_inputs({
            {in_data.data(), in_data.size() * sizeof(in_data.at(0))},
            {in_data.data(), in_data.size() * sizeof(in_data.at(0))},
        });
        instance.def_output(out_data.size() * sizeof(out_data.at(0)));

        if (!instance.execute({static_cast<int>(in_data.size()), 1, 1})) {
            std::cerr << "Execute shader program failed\n";
        }
        instance.get_output(out_data.data(), out_data.size() * sizeof(out_data.at(0)));
        // The instance will be automatically destroy here.
    }

    for (auto it: out_data) {
        std::cout << it << "\n";
    }
    return 0;
}
