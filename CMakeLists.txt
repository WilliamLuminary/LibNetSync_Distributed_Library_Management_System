cmake_minimum_required(VERSION 3.26)
project(LibNetSync_Distributed_Library_Management_System)

set(CMAKE_CXX_STANDARD 17)

# If you have additional libraries you need to link against, do it here:
# target_link_libraries(ServerM some_library)
# target_link_libraries(Client some_library)

add_executable(ServerM serverM.cpp)
add_executable(Client client.cpp)
add_executable(ServerS serverS.cpp)
add_executable(ServerL serverL.cpp)
add_executable(ServerH serverH.cpp)
