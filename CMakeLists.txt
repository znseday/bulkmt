cmake_minimum_required(VERSION 3.2)

if(WIN32 OR WIN64)
    message(STATUS "My Windows detected")
    project(bulkmt VERSION 0.0.1000)
    set(BOOST_ROOT "C:/Boost")
else()
    project(bulkmt VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})
endif()


if(WIN32 OR WIN64)
    add_executable(bulkmt main.cpp bulkmt.cpp bulkmt.h)
else()
    add_executable(bulkmt main.cpp bulkmt.cpp)
endif()


if(WIN32 OR WIN64)
    set_target_properties(bulkmt PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
			COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra")
else()
    set_target_properties(bulkmt PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra")

    set(CMAKE_BUILD_TYPE Release)
endif()

install(TARGETS bulkmt RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT "znseday")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

#enable_testing()

#add_test(my_tests_ my_tests)
