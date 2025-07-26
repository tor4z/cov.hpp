#include "mat.hpp"

#define COV_VULKAN_VALIDATION
#define COV_IMPLEMENTATION
#include "cov.hpp"


int main()
{
    // suppose we run this program on build dir
    // const std::string shader_path{"../examples/shader/multi_layer_matmul.comp.spv"};
    const std::string shader_path{"../examples/shader/multi_layer_matmul2.comp.spv"};

    Mat A{2, 2};
    Mat B{2, 2};
    Mat C{2, 2};
    Mat D{2, 2};

    B << 1.f, 1.f, 1.f, 1.f;
    A << 1.f, 1.f, 1.f, 1.f;
    C << 1.f, 1.f, 1.f, 1.f;

    cov::App::init("MutiLayerMatmul");

    {
        // core code
        auto instance{cov::App::new_instance()};
        instance.load_shader(shader_path);
        instance.set_inputs({
            {A.ptr(), A.bytes()},
            {B.ptr(), B.bytes()},
            {C.ptr(), C.bytes()},
        });
        instance.def_output(D.bytes());

        if (!instance.execute({D.row, D.col, 1})) {
            std::cerr << "Execute shader program failed\n";
        }
        instance.get_output(D.ptr(), D.bytes());
        // The instance will be automatically destroy here.
    }

    std::cout << "A: \n" << A << "\n";
    std::cout << "B: \n" << B << "\n";
    std::cout << "C: \n" << C << "\n";
    std::cout << "D: \n" << D << "\n";
    return 0;
}
