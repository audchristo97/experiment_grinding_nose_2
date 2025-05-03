cmake_minimum_required(VERSION 3.8)
project(aurora_ndi_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

# Include header directories of the Aurora API here
include_directories("/home/a-tamby/experiment_grinding_nose/sensors_experiment/aurora_api/library/include")

# Specify the source directory of the Aurora API
set(AURORA_API_SRC_DIR "/home/a-tamby/experiment_grinding_nose/sensors_experiment/aurora_api/")

# Set the path to your custom libraries
set(AURORA_API_LIB_DIR "/home/a-tamby/experiment_grinding_nose/sensors_experiment/aurora_api/build/linux")

# Add library path
link_directories(${AURORA_API_LIB_DIR})

# Export the library path during build
set(ENV{LD_LIBRARY_PATH} ${AURORA_API_LIB_DIR}:$ENV{LD_LIBRARY_PATH})

# # Ensure that the Aurora API is compiled before compiling the ROS 2 workspace
# include(ExternalProject)
# ExternalProject_Add(
#   aurora_api_build
#   PREFIX $(CMAKE_BINARY_DIR)/_deps
#   # The path to the Aurora API directory where MakeFile exists
#   SOURCE_DIR ${AURORA_API_SRC_DIR}
#   # Clean the API before compiling
#   CONFIGURE_COMMAND make -C ${AURORA_API_SRC_DIR} clean
#   # Command to build the API (run at the root directory)
#   BUILD_COMMAND make -C ${AURORA_API_SRC_DIR} # Run make from the source directory
#   INSTALL_COMMAND ""  # Skip install command
# )

# Once the API is compiled, collect the object files
file(GLOB AURORA_OBJECT_FILES "/home/a-tamby/experiment_grinding_nose/sensors_experiment/aurora_api/build/linux/obj/library/src/*.o")


# Add ROS 2 executable files here (i.e. ROS 2 node executable files)
add_executable(aurora_talker src/aurora_node.cpp)

# Link ROS 2 dependencies
ament_target_dependencies(aurora_talker rclcpp std_msgs)

# Link the Aurora API object files to the ROS 2 node
# target_link_libraries(aurora_talker 
#   ${AURORA_API_LIB_DIR}/libndicapi.so
#   ${AURORA_API_LIB_DIR}/libcapitogst.so
#   ${AURORA_API_LIB_DIR}/libndielems.so
#   ${AURORA_OBJECT_FILES}
# )
target_link_libraries(aurora_talker
  ${AURORA_API_LIB_DIR}/libndicapi.so
  ${AURORA_API_LIB_DIR}/libcapitogst.so
  ${AURORA_API_LIB_DIR}/libndielems.so
  ${AURORA_OBJECT_FILES}
)

# Export libraries to be available globally
ament_export_libraries(
  ${AURORA_API_LIB_DIR}/libndicapi.so
  ${AURORA_API_LIB_DIR}/libcapitogst.so
  ${AURORA_API_LIB_DIR}/libndielems.so
  )

# Install executables
install(TARGETS
  aurora_talker
  DESTINATION lib/${PROJECT_NAME}
)

# Ensure that the sroms folder is shared with your ROS node
install(DIRECTORY ${AURORA_API_SRC_DIR}/sroms 
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
