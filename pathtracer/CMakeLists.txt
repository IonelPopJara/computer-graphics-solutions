cmake_minimum_required ( VERSION 3.10 )

project ( pathtracer )

find_package ( embree 4 REQUIRED CONFIG )

function(copy_embree_runtime target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:embree>"
                "${EMBREE_ROOT_DIR}/bin/tbb12.dll"
                "${EMBREE_ROOT_DIR}/bin/tbbmalloc.dll"
                "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endif()
endfunction()

find_package ( OpenMP REQUIRED COMPONENTS CXX )

# Find *all* shaders.
file(GLOB_RECURSE SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
)
# Separate filter for shaders.
source_group("Shaders" FILES ${SHADERS})

# Build and link executable.
add_executable ( ${PROJECT_NAME}
    main.cpp
    Pathtracer.h
    Pathtracer.cpp
    sampling.h
    sampling.cpp
    HDRImage.h
    HDRImage.cpp
    embree.h
    embree.cpp
    material.h
    material.cpp
    ${SHADERS}
    )

target_link_libraries ( ${PROJECT_NAME} labhelper embree OpenMP::OpenMP_CXX )
config_build_output()
copy_embree_runtime(${PROJECT_NAME})

if(BUILD_TESTING)
    add_executable(pathtracer-embree-test tests/embree_test.cpp embree.cpp)
    target_include_directories(pathtracer-embree-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(pathtracer-embree-test labhelper embree)
    copy_windows_runtime(pathtracer-embree-test)
    copy_embree_runtime(pathtracer-embree-test)
    add_test(NAME pathtracer-embree COMMAND pathtracer-embree-test)
endif()
