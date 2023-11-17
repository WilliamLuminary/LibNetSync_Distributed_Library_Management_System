cmake_minimum_required(VERSION 3.26)
project(LibNetSync_Distributed_Library_Management_System)

set(CMAKE_CXX_STANDARD 17)

# If you have additional libraries you need to link against, do it here:
# target_link_libraries(ServerM some_library)
# target_link_libraries(client some_library)

add_executable(serverM serverM.cpp
        serverM.h)
add_executable(client client.cpp client.h)
add_executable(serverS serverS.cpp serverS.h)
add_executable(serverL serverL.cpp serverL.h)
add_executable(serverH serverH.cpp serverH.h)
