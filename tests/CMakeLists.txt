cmake_minimum_required(VERSION 3.16)
project(poolparty-tests LANGUAGES CXX)

# --------------------------------------------------------------------------------------------------------
# Create executable
# --------------------------------------------------------------------------------------------------------

add_executable(${PROJECT_NAME})
add_executable(poolparty::tests ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

if (NOT MSVC AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -pedantic -pedantic-errors -Wfatal-errors)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-warning-option -Wno-missing-field-initializers -Wno-cast-function-type)
endif()

# --------------------------------------------------------------------------------------------------------
# Add Sources
# --------------------------------------------------------------------------------------------------------

file(GLOB src "*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${src})

# --------------------------------------------------------------------------------------------------------
# Link Dependencies 
# --------------------------------------------------------------------------------------------------------

include("../cmake/cpm.cmake")

CPMFindPackage(
  NAME           ut
  GIT_TAG        fb06064
  GIT_REPOSITORY "https://github.com/boost-ext/ut"
  OPTIONS        "BOOST_UT_DISABLE_MODULE ON"
)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::ut cr::poolparty)

# --------------------------------------------------------------------------------------------------------
# Add Test-Target
# --------------------------------------------------------------------------------------------------------

include(CTest)

add_test(NAME poolparty-tests COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
