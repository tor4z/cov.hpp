#include "mat.hpp"

#define COV_VULKAN_VALIDATION
#define COV_IMPLEMENTATION
#include "cov.hpp"


int main()
{
    const std::string shader_path{"../examples/shader/matmul.comp.spv"};  // suppose we run this program on build dir

    Mat A{2, 2};
    Mat B{2, 2};
    Mat C{2, 2};

    A << 1.1f, 2.2f, 3.3f, 4.4f;
    B << 5.5f, 6.6f, 7.7f, 8.8f;

    cov::App::init("Matmul");

    {
        // core code
        auto instance{cov::App::new_instance()};
        instance.load_shader(shader_path);
        instance.set_inputs({
            {A.ptr(), A.bytes()},
            {B.ptr(), B.bytes()}
        });
        instance.def_output(C.bytes());

        if (!instance.execute({8, 8, 1})) {
            std::cerr << "Execute shader program failed\n";
        }
        instance.get_output(C.ptr(), C.bytes());
        // The instance will be automatically destroy here.
    }

    std::cout << "A: \n" << A << "\n";
    std::cout << "B: \n" << B << "\n";
    std::cout << "C: \n" << C << "\n";
    return 0;
}
