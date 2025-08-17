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
    Mat D{2, 2};
    Mat E{2, 2};

    A << 1.1f, 2.2f, 3.3f, 4.4f;
    B << 5.5f, 6.6f, 7.7f, 8.8f;
    D << 1.2f, 2.1f, 3.1f, 4.1f;

    cov::Vulkan::init("Matmul");

    {
        // create instance
        auto instance{cov::Vulkan::new_instance()};
        // create data mapping
        auto A_mapping{instance.add_mem_mapping(A.bytes())};
        auto B_mapping{instance.add_mem_mapping(B.bytes())};
        auto C_mapping{instance.add_mem_mapping(C.bytes())};
        auto D_mapping{instance.add_mem_mapping(D.bytes())};
        auto E_mapping{instance.add_mem_mapping(E.bytes())};

        {
            // buid compute pipeline
            instance.add_transfer_pass()
                ->to_device(A_mapping)
                ->to_device(B_mapping)
                ->to_device(D_mapping)
                ->build();

            instance.add_compute_pass()
                ->load_shader(shader_path)
                ->set_inputs({A_mapping, B_mapping})
                ->set_outputs({C_mapping})
                ->set_workgroup_dims(C.row, C.col, 1)
                ->build();

            instance.add_compute_pass()
                ->load_shader(shader_path)
                ->set_inputs({C_mapping, D_mapping})
                ->set_outputs({E_mapping})
                ->set_workgroup_dims(E.row, E.col, 1)
                ->build();

            instance.add_transfer_pass()
                ->from_device(C_mapping)
                ->from_device(E_mapping)
                ->build();
        }

        {
            // compute with data 
            A_mapping->copy_from(A.ptr(), A.bytes());
            B_mapping->copy_from(B.ptr(), B.bytes());
            D_mapping->copy_from(D.ptr(), D.bytes());
            if (!instance.execute()) {
                std::cerr << "Execute shader program failed\n";
            }
            C_mapping->copy_to(C.ptr(), C.bytes());
            E_mapping->copy_to(E.ptr(), E.bytes());

            std::cout << "A: \n" << A << "\n";
            std::cout << "B: \n" << B << "\n";
            std::cout << "C = A * B: \n" << C << "\n";
            std::cout << "D: \n" << D << "\n";
            std::cout << "E = (A * B) * D: \n" << E << "\n";
        }
        // The instance will be automatically destroy here.
    }

    return 0;
}
