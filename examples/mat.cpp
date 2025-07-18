#include "mat.hpp"
#include <assert.h>


Mat::Mat(int col, int row)
    : dims(2)
    , col(col)
    , row(row)
    , data_offset_(sizeof(int) * 3)
{
    byte_data_.resize(data_offset_ + col * row * sizeof(float));
    *(byte_data_.data()) = dims;
    *(byte_data_.data() + sizeof(int)) = col;
    *(byte_data_.data() + sizeof(int) * 2) = row;
}

void* Mat::ptr()
{
    return reinterpret_cast<void*>(byte_data_.data());
}


size_t Mat::bytes()
{
    return data_offset_ + col * row * sizeof(float);
}

MatInitializer Mat::operator<<(float v)
{
    MatInitializer mi{
        .byte_data = byte_data_,
        .idx = 1,
        .data_offset_ = data_offset_,
        .num_element_ = row * col
    };

    *reinterpret_cast<float*>(byte_data_.data() + mi.data_offset_) = v;
    return mi;
}

float Mat::at(int i) const
{
    return *reinterpret_cast<const float*>(byte_data_.data() + data_offset_ + sizeof(float) * i);
}


MatInitializer& MatInitializer::operator,(float v)
{
    assert(idx < num_element_);
    *reinterpret_cast<float*>(byte_data.data() + data_offset_ + sizeof(v) * idx) = v;
    ++idx;
    return *this;
}


std::ostream& operator<<(std::ostream& os, const Mat& mat)
{
    int i{0};
    os << "[";
    for (int r = 0; r < mat.row; ++r) {
        if (r > 0) os << " ";
        for (int c = 0; c < mat.col; ++c) {
            os << mat.at(i++);
            if (c < mat.col - 1) os << ", ";
        }
        if (r < mat.row - 1) os << "\n";
    }

    os << "]";
    return os;
}
