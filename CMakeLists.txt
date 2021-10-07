cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(MAFUinput)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -nostdlib -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions -Wno-unknown-pragmas")

include_directories(
)

file(GLOB_RECURSE mafu_src
	"./src/*.c"
)

add_executable(${PROJECT_NAME}
	${mafu_src}
)

target_link_libraries( ${PROJECT_NAME}
  SceKernelSuspendForDriver_stub
  ScePowerForDriver_stub 
  SceDisplayForDriver_stub
  SceProcessmgrForDriver_stub 
  SceTouchForDriver_stub
  SceCtrlForDriver_stub
  SceDebugForDriver_stub
  SceSysmemForDriver_stub 
  SceSysclibForDriver_stub
  SceIofilemgrForDriver_stub
  SceThreadmgrForDriver_stub 
  taihenForKernel_stub
  taihenModuleUtils_stub
)

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME} CONFIG exports.yml)

add_custom_target(copy
  COMMAND curl -T ${PROJECT_NAME}.skprx ftp://192.168.1.3:1337/ur0:/tai/${PROJECT_NAME}.skprx
  DEPENDS ${PROJECT_NAME}.skprx
)

add_custom_target(deploy
  COMMAND curl -T ${PROJECT_NAME}.skprx ftp://192.168.1.3:1337/ur0:/tai/${PROJECT_NAME}.skprx
  COMMAND echo reboot | nc 192.168.1.3 1338
  DEPENDS ${PROJECT_NAME}.skprx
)