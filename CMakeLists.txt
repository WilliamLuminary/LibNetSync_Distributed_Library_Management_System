cmake_minimum_required(VERSION 3.26)
project(LibNetSync_Distributed_Library_Management_System)

set(CMAKE_CXX_STANDARD 17)

# If you have additional libraries you need to link against, do it here:
# target_link_libraries(ServerM some_library)
# target_link_libraries(client some_library)

add_executable(serverM ee450_Li_Yaxing_yaxingl/serverM.cpp
        ee450_Li_Yaxing_yaxingl/serverM.h)
add_executable(client ee450_Li_Yaxing_yaxingl/client.cpp ee450_Li_Yaxing_yaxingl/client.h)
add_executable(serverS ee450_Li_Yaxing_yaxingl/serverS.cpp ee450_Li_Yaxing_yaxingl/serverS.h)
add_executable(serverL ee450_Li_Yaxing_yaxingl/serverL.cpp ee450_Li_Yaxing_yaxingl/serverL.h)
add_executable(serverH ee450_Li_Yaxing_yaxingl/serverH.cpp ee450_Li_Yaxing_yaxingl/serverH.h)