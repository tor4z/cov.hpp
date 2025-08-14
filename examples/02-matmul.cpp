#include "mat.hpp"

#define COV_VULKAN_VALIDATION
#define COV_IMPLEMENTATION
#include "cov.hpp"


int main()
{
    const std::string shader_path{"../examples/shader/matmul.comp.spv"};  // suppose we run this program on build dir

    Mat A1{2, 2};
    Mat A2{2, 2};
    Mat B{2, 2};
    Mat C{2, 2};

    A1 << 1.1f, 2.2f, 3.3f, 4.4f;
    A2 << 1.2f, 2.1f, 3.1f, 4.1f;
    B << 5.5f, 6.6f, 7.7f, 8.8f;

    cov::App::init("Matmul");

    {
        // define compute
        auto instance{cov::App::new_instance()};
        auto A_id{instance.add_mem_mapping(A1.bytes())};
        auto B_id{instance.add_mem_mapping(B.bytes())};
        auto C_id{instance.add_mem_mapping(C.bytes())};
        instance.add_pass(shader_path, {A_id, B_id}, {C.row, C.col, 1});

        {
            // compute with data
            instance.to_device(A_id, A1.ptr(), A1.bytes());
            instance.to_device(B_id, B.ptr(), B.bytes());

            if (!instance.execute()) {
                std::cerr << "Execute shader program failed\n";
            }
            instance.from_device(C_id, C.ptr(), C.bytes());

            std::cout << "A: \n" << A1 << "\n";
            std::cout << "B: \n" << B << "\n";
            std::cout << "C: \n" << C << "\n";
        }

        {
            // compute with another data
            instance.to_device(A_id, A2.ptr(), A2.bytes());
            instance.to_device(B_id, B.ptr(), B.bytes());

            if (!instance.execute()) {
                std::cerr << "Execute shader program failed\n";
            }
            instance.from_device(C_id, C.ptr(), C.bytes());

            std::cout << "A: \n" << A2 << "\n";
            std::cout << "B: \n" << B << "\n";
            std::cout << "C: \n" << C << "\n";
        }
        // The instance will be automatically destroy here.
    }

    return 0;
}
