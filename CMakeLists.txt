cmake_minimum_required(VERSION 3.8)
project(ros2_shm_demo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rclcpp REQUIRED)

# only needed to build a separate iceoryx subscriber
# which is used to verify Shared Memory communication 
find_package(iceoryx_posh CONFIG REQUIRED)

include_directories(include)

set(msg_files
  "msg/ShmTopic.msg"
)

rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
)

ament_export_dependencies(rosidl_default_runtime)

add_executable(talker
  src/talker.cpp
)

ament_target_dependencies(talker
  "rclcpp"
)

# we want to use the messages from the same package
# would not be needed if we build the messages in a separate package
rosidl_target_interfaces(talker
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

install(TARGETS talker
  DESTINATION lib/${PROJECT_NAME})

add_executable(listener
  src/listener.cpp
)

ament_target_dependencies(listener
  "rclcpp"
)

rosidl_target_interfaces(listener
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

install(TARGETS listener
  DESTINATION lib/${PROJECT_NAME})

# we build an iceoryx subscriber app that also listens to the topic
add_executable(iox_subscriber
  src/iox_subscriber.cpp
)

target_link_libraries(iox_subscriber
    iceoryx_posh::iceoryx_posh
)

rosidl_target_interfaces(iox_subscriber
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

install(TARGETS iox_subscriber
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
