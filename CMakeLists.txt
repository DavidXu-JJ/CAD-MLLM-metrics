cmake_minimum_required(VERSION 3.10.0)

# Template taken from https://github.com/nmwsharp/gc-polyscope-project-template
project(cad_mllm_metrics)

# Configure output locations
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Print the build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build, options are: Debug Release" FORCE)
endif()
message(STATUS "cmake build type: ${CMAKE_BUILD_TYPE}")

# Configure the compiler This is a basic, decent setup that should do something
# sane on most compilers

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}"
                                                 STREQUAL "GNU")

  # using Clang (linux or apple) or GCC
  message("Using clang/gcc compiler flags")
  set(BASE_CXX_FLAGS "-std=c++14 -Wall -Wextra")
  set(DISABLED_WARNINGS
      " -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-missing-braces -Wno-unused-private-field"
  )
  set(TRACE_INCLUDES " -H -Wno-error=unused-command-line-argument")

  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    message("Setting clang-specific options")
    set(BASE_CXX_FLAGS "${BASE_CXX_FLAGS} -ferror-limit=3 -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address -fno-limit-debug-info")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message("Setting gcc-specific options")
    set(BASE_CXX_FLAGS "${BASE_CXX_FLAGS} -fmax-errors=5")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    set(DISABLED_WARNINGS
        "${DISABLED_WARNINGS} -Wno-maybe-uninitialized -Wno-format-zero-length -Wno-unused-but-set-parameter -Wno-unused-but-set-variable"
    )
  endif()

  set(CMAKE_CXX_FLAGS "${BASE_CXX_FLAGS} ${DISABLED_WARNINGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  message("Using Visual Studio compiler flags")
  set(BASE_CXX_FLAGS "${BASE_CXX_FLAGS} /W4")
  set(BASE_CXX_FLAGS "${BASE_CXX_FLAGS} /MP") # parallel build
  set(DISABLED_WARNINGS "${DISABLED_WARNINGS} /wd\"4267\"") # ignore conversion
                                                            # to smaller type
                                                            # (fires more
                                                            # aggressively than
                                                            # the gcc version,
                                                            # which is annoying)
  set(DISABLED_WARNINGS "${DISABLED_WARNINGS} /wd\"4244\"") # ignore conversion
                                                            # to smaller type
                                                            # (fires more
                                                            # aggressively than
                                                            # the gcc version,
                                                            # which is annoying)
  set(DISABLED_WARNINGS "${DISABLED_WARNINGS} /wd\"4305\"") # ignore truncation
                                                            # on initialization
  set(CMAKE_CXX_FLAGS "${BASE_CXX_FLAGS} ${DISABLED_WARNINGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

  add_definitions(/D "_CRT_SECURE_NO_WARNINGS")
  add_definitions(-DNOMINMAX)
  add_definitions(-D_USE_MATH_DEFINES)
else()
  # unrecognized
  message(FATAL_ERROR "Unrecognized compiler [${CMAKE_CXX_COMPILER_ID}]")
endif()

# == Deps
add_subdirectory(deps/polyscope)
add_subdirectory(deps/cgal)
find_package(CGAL REQUIRED)

# == Build our project stuff

# To change the name of your executable, change "gc_project" in the lines below
# to whatever you want
add_executable(mesh_segment src/mesh_segment.cpp)
add_executable(dangling_edge src/dangling_edge.cpp)
add_executable(flux_enclosure_error src/flux_enclosure_error.cpp)
add_executable(self_intersection src/self_intersection.cpp)
target_include_directories(mesh_segment
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories(dangling_edge
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories(flux_enclosure_error
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories(self_intersection
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
# add the args.hxx project which we use for command line args
target_include_directories(
  mesh_segment PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/args")
target_include_directories(
  dangling_edge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/args")
target_include_directories(
  flux_enclosure_error PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/args")
target_include_directories(
  self_intersection PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/args")
target_link_libraries(mesh_segment CGAL::CGAL)
target_link_libraries(dangling_edge CGAL::CGAL)
target_link_libraries(flux_enclosure_error CGAL::CGAL)
target_link_libraries(self_intersection CGAL::CGAL)
