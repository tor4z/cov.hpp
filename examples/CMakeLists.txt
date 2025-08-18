add_compile_options(-g)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(matmul
    01-matmul.cpp
    mat.cpp
)

target_link_libraries(matmul
    vulkan
)


add_executable(multi_step_matmul
    02-multi_step_matmul.cpp
    mat.cpp
)

target_link_libraries(multi_step_matmul
    vulkan
)
