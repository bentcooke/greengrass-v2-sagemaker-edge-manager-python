#
# Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#

cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)

project(sagemaker_edge_agent_client_example C CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")

include(FetchContent)

FetchContent_Declare(
  grpc
  GIT_REPOSITORY https://github.com/grpc/grpc.git
  GIT_TAG "v1.32.0")
FetchContent_MakeAvailable(grpc)

# define sagemaker_edge_agent proto file
get_filename_component(sagemaker_edge_agent_proto "sagemaker_edge_agent.proto" ABSOLUTE)
get_filename_component(sagemaker_edge_agent_proto_path "${sagemaker_edge_agent_proto}" PATH)

set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/sagemaker_edge_agent.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/sagemaker_edge_agent.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/sagemaker_edge_agent.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/sagemaker_edge_agent.grpc.pb.h")

# include generated gRPC sources
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(protoc $<TARGET_FILE:protoc>)
set(grpc_cpp_plugin $<TARGET_FILE:grpc_cpp_plugin>)

add_custom_command(
  OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
  COMMAND ${protoc} ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" --cpp_out "${CMAKE_CURRENT_BINARY_DIR}" -I "${sagemaker_edge_agent_proto_path}"
          --plugin=protoc-gen-grpc="${grpc_cpp_plugin}" "${sagemaker_edge_agent_proto}"
  DEPENDS "${sagemaker_edge_agent_proto}")

# build sagemaker_edge_agent_client
add_executable(
  sagemaker_edge_agent_client_example
  shm.hh shm.cc ${proto_srcs} ${grpc_srcs}
  sagemaker_edge_agent_example.cc sagemaker_edge_agent_client.hh sagemaker_edge_agent_client.cc)

target_link_libraries(sagemaker_edge_agent_client_example grpc++_reflection libprotobuf)