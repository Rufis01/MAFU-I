cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{DOLCESDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define DOLCESDK to point to your SDK path!")
  endif()
endif()

project(MAFUinput)
include("${DOLCESDK}/share/dolce.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

include_directories(
)

add_executable(${PROJECT_NAME}
  main.c
)

target_link_libraries( ${PROJECT_NAME}
  SceCtrlForDriver_stub
  SceDebugForDriver_stub
  SceSysmemForDriver_stub 
  SceSysclibForDriver_stub
  SceIofilemgrForDriver_stub
  SceThreadmgrForDriver_stub 
  taihenForKernel_stub
)

dolce_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME} CONFIG exports.yml)

add_custom_target(copy
  COMMAND curl -T ${PROJECT_NAME}.skprx ftp://192.168.1.3:1337/ur0:/tai/${PROJECT_NAME}.skprx
  DEPENDS ${PROJECT_NAME}.skprx
)

add_custom_target(deploy
  COMMAND curl -T ${PROJECT_NAME}.skprx ftp://192.168.1.3:1337/ur0:/tai/${PROJECT_NAME}.skprx
  COMMAND echo reboot | nc 192.168.1.3 1338
  DEPENDS ${PROJECT_NAME}.skprx
)