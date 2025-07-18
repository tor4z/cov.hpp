#ifndef COV_MAT_H_
#define COV_MAT_H_

#include <ostream>
#include <vector>


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
    float at(int i) const;
    MatInitializer operator<<(float v);

    const int dims;
    const int col;
    const int row;
private:
    float* data_;
    int data_offset_;
    std::vector<char> byte_data_;
}; // struct Mat

std::ostream& operator<<(std::ostream& os, const Mat& mat);

#endif // COV_MAT_H_
