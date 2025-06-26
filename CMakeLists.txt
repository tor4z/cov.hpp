cmake_minimum_required(VERSION 3.10.0)

project(compute_vulkan)

add_compile_options(-g)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    vulkan
)
