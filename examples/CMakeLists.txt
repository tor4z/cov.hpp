add_compile_options(-g)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(basic_usage
    01-basic_usage.cpp
)

target_link_libraries(basic_usage
    vulkan
)


add_executable(matmul
    02-matmul.cpp
    mat.cpp
)

target_link_libraries(matmul
    vulkan
)

add_executable(multi_layer_matmul
    03-multi_layer_matmul.cpp
    mat.cpp
)

target_link_libraries(multi_layer_matmul
    vulkan
)

