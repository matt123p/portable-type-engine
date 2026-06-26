cmake_minimum_required(VERSION 3.16)
project(PortableTypeEngine VERSION 2.0.1 LANGUAGES C CXX)

add_library(pte src/pte/pte.c)
target_include_directories(pte PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(pte PUBLIC c_std_99)

option(PTE_BUILD_WINDOWS_TEST "Build the BMP-producing Windows example" OFF)
if(PTE_BUILD_WINDOWS_TEST)
    add_executable(pte_windows_test
        examples/windows-test/test.cpp
        examples/fonts/roboto_regular.c
    )
    target_link_libraries(pte_windows_test PRIVATE pte)
endif()

set(LVGL_DIR "" CACHE PATH "Path to an external LVGL 9 source checkout")
option(PTE_BUILD_LVGL_TESTS "Build the LVGL adapter test" OFF)
if(PTE_BUILD_LVGL_TESTS)
    if(NOT LVGL_DIR)
        message(FATAL_ERROR "Set LVGL_DIR to an LVGL 9 source checkout")
    endif()
    set(LV_BUILD_CONF_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/lvgl/lv_conf.h
        CACHE FILEPATH "LVGL configuration used by PTE tests" FORCE)
    set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Do not build LVGL demos" FORCE)
    set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Do not build LVGL examples" FORCE)
    add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl EXCLUDE_FROM_ALL)
    add_library(pte_lvgl src/lvgl/lv_pte.c)
    target_include_directories(pte_lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(pte_lvgl PUBLIC lvgl)

    enable_testing()
    add_executable(test_lv_pte tests/lvgl/test_lv_pte.c)
    target_link_libraries(test_lv_pte PRIVATE pte_lvgl)
    add_test(NAME lv_pte COMMAND test_lv_pte)
endif()
