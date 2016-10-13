project (cool)
cmake_minimum_required(VERSION 2.8)

## #######################################################################
#
# Input variables
#
#  COOL_HOME         top level source directory
#  COOL_LIBRARY_DIR  location of build library
#  COOL_LIBRARY_NAME the name of the library
#  COOL_TEST_DIR     location of built tests, if enabled
#  COOL_BUILD_TEST   build test programs (true/false)
#  COOL_BUILD_DOC    build HTML documentation, requires doxygen (true/false)
#  COOL_DOC_DIR      location of generated documentation, if built
#
# Output variables:
#  COOL_LIBS         cool library and dependent libraries
#  COOL_INCLUDE_DIR  include path for cool header files
#  COOL_VERSION      version number
#  COOL_SOVERSION    SOVERSION of the dynamic library (for Unixy platforms)
#
## #######################################################################

# version number and SOVERSION
set (COOL_NAME cool2)
set (COOL_VERSION "2.0.0")
set (COOL_SOVERSION "2")
set (STATIC_LIB_SFX -static)
set (DYNAMIC_LIB_SFX -dynamic)

# -- platform detection
include (cmake/platform)

# -- defaults if input variables aren't set

if (NOT DEFINED COOL_LIBRARY_NAME)
  set (COOL_LIBRARY_NAME ${COOL_NAME})
endif()
if (NOT DEFINED COOL_BUILD_TEST)
  set (COOL_BUILD_TEST true)
endif()
if (NOT DEFINED COOL_BUILD_DOC)
  set (COOL_BUILD_DOC true)
endif()

# --- directories
if (NOT DEFINED COOL_HOME)
  set (COOL_HOME ${CMAKE_CURRENT_LIST_DIR})
endif()

if (NOT DEFINED COOL_LIBRARY_DIR)
  set (COOL_LIBRARY_DIR ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT DEFINED COOL_BINARY_DIR)
  set (COOL_BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
endif()

if (NOT DEFINED COOL_TEST_DIR)
  if (WINDOWS)
    set (COOL_TEST_DIR ${COOL_BINARY_DIR})
  else()
    set (COOL_TEST_DIR ${CMAKE_BINARY_DIR}/test)
  endif()
endif()
if (NOT DEFINED COOL_DOC_DIR)
  set (COOL_DOC_DIR ${CMAKE_BINARY_DIR}/doc)
endif()

message (STATUS "  build test programs: ${COOL_BUILD_TEST}")
message (STATUS "  build documentation: ${COOL_BUILD_DOC}")

# set output variables
if (OSX)
  set (COOL_LINK_LIBRARIES)
elseif (LINUX)
  set (COOL_LINK_LIBRARIES dispatch pthread)
else()
  set (COOL_LINK_LIBRARIES)
endif()

set (COOL_STATIC_LIBRARY_NAME ${COOL_LIBRARY_NAME}${STATIC_LIB_SFX})
set (COOL_DYNAMIC_LIBRARY_NAME ${COOL_LIBRARY_NAME}${DYNAMIC_LIB_SFX})
set (COOL_LIBS ${COOL_LIBRARY_NAME} ${COOL_LINK_LIBRARIES})

get_directory_property(hasParent PARENT_DIRECTORY)
if (hasParent)
  set (COOL_INCLUDE ${COOL_HOME}/include PARENT_SCOPE)
  set (COOL_LIBS ${COOL_LIBS} PARENT_SCOPE)
  set (COOL_VERSION ${COOL_VERSION} PARENT_SCOPE)
  set (COOL_SOVERSION ${COOL_SOVERSION} PARENT_SCOPE)
endif()

include_directories(/usr/local/include ${COOL_HOME}/include)
if (WINDOWS)
  include_directories(${xdispatch_DIR}/libdispatch/include)
endif()

if (OSX)
  set(DISABLED_WARNINGS -Wno-undefined-bool-conversion)
endif()

if (WINDOWS)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /EHsc /MTd")
else()
  set (CMAKE_CXX_FLAGS "-std=c++11 -g ${DISABLED_WARNINGS} -fpermissive")
endif()

# ------ COOL FEATURES

if (NOT COOL_ASYNC_PLATFORM)
  if (WINDOWS)
    set (COOL_ASYNC_PLATFORM winwtp)
    set (COOL_ASYNC_PLATFORM_DEFINE COOL_ASYNC_PLATFORM_WINWTP)
  else()
    set (COOL_ASYNC_PLATFORM gcd)
    set (COOL_ASYNC_PLATFORM_DEFINE COOL_ASYNC_PLATFORM_GCD)
  endif()
endif()

set (COOL_SRCS
     ${COOL_HOME}/impl/src/async.cpp
     ${COOL_HOME}/impl/src/named.cpp
     ${COOL_HOME}/impl/src/${COOL_ASYNC_PLATFORM}/runner.cpp
# -- old stuff
     ${COOL_HOME}/lib/miscellaneous.cpp
     ${COOL_HOME}/lib/vow.cpp
)
if (OSX OR LINUX)
  set (COOL_SRCS ${COOL_SRCS} ${COOL_HOME}/lib/ip_address.cpp)
endif()

set (COOL_HEADERS
    ${COOL_HOME}/include/cool2/named.h
    ${COOL_HOME}/include/cool2/async.h
    ${COOL_HOME}/include/cool2/async/runner.h
    ${COOL_HOME}/include/cool2/async/task.h
#-- old stuff
    ${COOL_HOME}/include/cool/cool.h
    ${COOL_HOME}/include/cool/exception.h
    ${COOL_HOME}/include/cool/miscellaneous.h
    ${COOL_HOME}/include/cool/vow.h
    ${COOL_HOME}/include/cool/binary.h
)

if (OSX OR LINUX)
  set (COOL_HEADERS ${COOL_HEADERS} ${COOL_HOME}/include/cool/ip_address.h)
endif()

set (COOL_IMPL_HEADERS
    ${COOL_HOME}/include/cool2/impl/platform.h
    ${COOL_HOME}/include/cool2/impl/traits.h
    ${COOL_HOME}/include/cool2/async/impl/task.h
    ${COOL_HOME}/impl/include/entrails/runner.h
    ${COOL_HOME}/impl/include/entrails/${COOL_ASYNC_PLATFORM}/runner.h
    #-- old stuff
    ${COOL_HOME}/include/cool/entrails/vow.h
    ${COOL_HOME}/include/cool/entrails/platform.h
    ${COOL_HOME}/include/cool/entrails/traits.h
)

if (OSX OR LINUX)
  set (COOL_IMPL_HEADERS ${COOL_IMPL_HEADERS} ${COOL_HOME}/include/cool/entrails/ip_address.h)
endif()

source_group("Api Header Files" FILES ${COOL_HEADERS})
source_group("Impl Header Files" FILES ${COOL_IMPL_HEADERS})
source_group("Impl Source Files" FILES ${COOL_SRCS})

# -- dynamic library
add_library (${COOL_DYNAMIC_LIBRARY_NAME} SHARED ${COOL_SRCS} ${COOL_HEADERS} ${COOL_IMPL_HEADERS})
set_target_properties (${COOL_DYNAMIC_LIBRARY_NAME} PROPERTIES
    COMPILE_DEFINITIONS "${TARGET};COOL_BUILD;${COOL_ASYNC_PLATFORM_DEFINE}"
    OUTPUT_NAME ${COOL_NAME}
    VERSION ${COOL_VERSION}
    SOVERSION ${COOL_SOVERSION}
    FOLDER Cool2
)
if (WINDOWS)
  set_target_properties (${COOL_DYNAMIC_LIBRARY_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${COOL_BINARY_DIR}
      ARCHIVE_OUTPUT_DIRECTORY ${COOL_BINARY_DIR}
  )
else()
  set_target_properties (${COOL_DYNAMIC_LIBRARY_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${COOL_LIBRARY_DIR})
endif()
target_link_libraries (${COOL_DYNAMIC_LIBRARY_NAME} ${COOL_LINK_LIBRARIES})
target_include_directories (${COOL_DYNAMIC_LIBRARY_NAME} PRIVATE ${COOL_HOME}/impl/include)

# -- static library
add_library(${COOL_STATIC_LIBRARY_NAME} STATIC ${COOL_SRCS} ${COOL_HEADERS} ${COOL_IMPL_HEADERS})
set_target_properties(${COOL_STATIC_LIBRARY_NAME} PROPERTIES
  COMPILE_DEFINITIONS "COOL_STATIC;${TARGET};COOL_BUILD;${COOL_ASYNC_PLATFORM_DEFINE}"
  OUTPUT_NAME ${COOL_NAME}
  ARCHIVE_OUTPUT_DIRECTORY ${COOL_LIBRARY_DIR}
  FOLDER Cool2
)
target_include_directories (${COOL_STATIC_LIBRARY_NAME} PRIVATE ${COOL_HOME}/impl/include)

# --- Tests
set (TESTS
   2_runner
)

#if( NOT WINDOWS )
#  set (TESTS ${TESTS}
#    reader_writer
#  )
#endif()

# --- Doxygen documentation

if (COOL_BUILD_DOC)

  find_package(Doxygen)
  if (NOT Doxygen_FOUND)
    message(WARNING "Doxygen package not found, will not generate documentation")
  else ()
    make_directory (${COOL_DOC_DIR})
    configure_file(
      ${COOL_HOME}/cool.dox.in
      ${COOL_DOC_DIR}/cool.dox
      @ONLY
    )
    add_custom_command(
      OUTPUT ${COOL_DOC_DIR}/doc/html/index.html
      COMMAND ${DOXYGEN_EXECUTABLE} ${COOL_DOC_DIR}/cool.dox
      DEPENDS ${COOL_HEADERS}
      WORKING_DIRECTORY ${COOL_DOC_DIR}
    )
    add_custom_target(Documentation ALL DEPENDS ${COOL_DOC_DIR}/doc/html/index.html)
  endif()

endif()

# --- Tests

if (COOL_BUILD_TEST)

  if (NOT GTEST_LIBRARY)
    include (cmake/gtest)
  endif()

  foreach (test ${TESTS})

    set (test_target test_${test})
    add_executable (test_${test} ${COOL_HOME}/test/${test_target}.cpp)
    if ( WINDOWS )
      target_link_libraries(${test_target} ${GTEST_LIBRARY} ${GTEST_LIBRARY_MAIN} ${COOL_STATIC_LIBRARY_NAME} ${COOL_LINK_LIBRARIES}  )
    set_target_properties (${test_target} PROPERTIES COMPILE_DEFINITIONS COOL_STATIC)
    else()
      target_link_libraries(${test_target} ${GTEST_LIBRARY} ${GTEST_LIBRARY_MAIN} ${COOL_DYNAMIC_LIBRARY_NAME} ${COOL_LINK_LIBRARIES}  )
    endif()

    set_target_properties (${test_target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${COOL_TEST_DIR}
        FOLDER Tests
    )

  endforeach()

endif()
