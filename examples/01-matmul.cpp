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

    cov::Vulkan::init("Matmul");

    {
        // create instance
        auto instance{cov::Vulkan::new_instance()};
        // create data mapping
        auto A_mapping{instance.add_mem_mapping(A.bytes())};
        auto B_mapping{instance.add_mem_mapping(B.bytes())};
        auto C_mapping{instance.add_mem_mapping(C.bytes())};

        {
            // buid compute pipeline
            instance.add_transfer_pass()
                ->to_device(A_mapping)
                ->to_device(B_mapping)
                ->build();

            instance.add_compute_pass()
                ->load_shader_from_file(shader_path)
                ->use_mem_mapping({A_mapping, B_mapping, C_mapping})
                ->set_workgroup_dims(C.row, C.col, 1)
                ->set_mem_barrier({A_mapping, B_mapping})
                ->build();

            instance.add_transfer_pass()
                ->from_device(C_mapping)
                ->build();
        }

        {
            // compute with data 
            A_mapping->copy_from(A.ptr(), A.bytes());
            B_mapping->copy_from(B.ptr(), B.bytes());
            if (!instance.execute()) {
                std::cerr << "Execute shader program failed\n";
            }
            C_mapping->copy_to(C.ptr(), C.bytes());

            std::cout << "A: \n" << A << "\n";
            std::cout << "B: \n" << B << "\n";
            std::cout << "C = A * B: \n" << C << "\n";
        }
        // The instance will be automatically destroy here.
    }

    return 0;
}
