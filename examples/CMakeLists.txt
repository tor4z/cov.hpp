add_compile_options(-g)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(basic_usage
    01-basic_usage.cpp
)

target_link_libraries(basic_usage
    vulkan
)
