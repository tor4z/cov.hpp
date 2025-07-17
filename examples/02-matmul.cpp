#include <cassert>
#include <cstddef>
#include <vector>

#define COV_VULKAN_VALIDATION
#define COV_IMPLEMENTATION
#include "cov.hpp"


struct MatInitializer
{
    MatInitializer& operator,(float v);
    std::vector<char>& byte_data;
    int idx;
    int data_offset_;
    int num_element_;
}; // struct MatInitializer


struct Mat
{
    Mat(int col, int row);
    void* ptr();
    size_t bytes();
    float at(int i);
    MatInitializer operator<<(float v);
private:
    int dims_;
    int col_;
    int row_;
    float* data_;
    int data_offset_;
    std::vector<char> byte_data_;
}; // struct Mat



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

    std::cout << "A: " << A.at(0) << ", " << A.at(1) << ", " << A.at(2) << ", " << A.at(3) << "\n";
    std::cout << "B: " << B.at(0) << ", " << B.at(1) << ", " << B.at(2) << ", " << B.at(3) << "\n";
    std::cout << "C: " << C.at(0) << ", " << C.at(1) << ", " << C.at(2) << ", " << C.at(3) << "\n";
    return 0;
}


Mat::Mat(int col, int row)
    : dims_(2)
    , col_(col)
    , row_(row)
{
    byte_data_.resize(sizeof(int) * 3 + col * row * sizeof(float));
    *(byte_data_.data()) = dims_;
    *(byte_data_.data() + sizeof(int)) = col_;
    *(byte_data_.data() + sizeof(int) * 2) = row_;
}

void* Mat::ptr()
{
    return reinterpret_cast<void*>(byte_data_.data());
}


size_t Mat::bytes()
{
    return sizeof(int) * 3 + col_ * row_ * sizeof(float);
}

MatInitializer Mat::operator<<(float v)
{
    MatInitializer mi{
        .byte_data = byte_data_,
        .idx = 1,
        .data_offset_ = sizeof(int) * 3,
        .num_element_ = row_ * col_
    };

    *reinterpret_cast<float*>(byte_data_.data() + mi.data_offset_) = v;
    return mi;
}

float Mat::at(int i)
{
    return *reinterpret_cast<float*>(byte_data_.data() + sizeof(int) * 3 + sizeof(float) * i);
}


MatInitializer& MatInitializer::operator,(float v)
{
    assert(idx < num_element_);
    *reinterpret_cast<float*>(byte_data.data() + data_offset_ + sizeof(v) * idx) = v;
    ++idx;
    return *this;
}
