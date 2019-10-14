cmake_minimum_required(VERSION 2.8)

project (Affine)

find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

add_executable (Affine Affine.cxx)

target_link_libraries (Affine ${ITK_LIBRARIES})
