set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

if(WIN32)
    set(CROSS_PLATFORM_USER_DIR "$ENV{USERPROFILE}")
else()
    set(CROSS_PLATFORM_USER_DIR "$ENV{HOME}")
endif()
set(CROSSTOOL_PATH ${CROSS_PLATFORM_USER_DIR}/.niiet_aspect/riscv_gcc_windows/bin CACHE PATH "Cross toolchain root directory") 

# Look for GCC in path
function(find_toolchain)
  foreach(CROSS_PREFIX ${ARGV})
    find_program(CROSS_CC "${CROSS_PREFIX}gcc" "${CROSSTOOL_PATH}")
    find_program(CROSS_CXX "${CROSS_PREFIX}g++" "${CROSSTOOL_PATH}")
    find_program(CROSS_ASM "${CROSS_PREFIX}gcc" "${CROSSTOOL_PATH}")
    find_program(CROSS_OBJDUMP "${CROSS_PREFIX}objdump" "${CROSSTOOL_PATH}")
    find_program(CROSS_OBJCOPY "${CROSS_PREFIX}objcopy" "${CROSSTOOL_PATH}")
    find_program(CROSS_SIZE "${CROSS_PREFIX}size" "${CROSSTOOL_PATH}")
    if (CROSS_CC AND CROSS_CXX AND CROSS_OBJCOPY AND CROSS_OBJCOPY AND CROSS_SIZE)
      message("@@ found toolchain ${CROSS_CC}")
      set(CMAKE_C_COMPILER ${CROSS_CC} PARENT_SCOPE)
      set(CMAKE_CXX_COMPILER ${CROSS_CXX} PARENT_SCOPE)
      set(CMAKE_ASM_COMPILER ${CROSS_CC} PARENT_SCOPE)
      set(CMAKE_OBJDUMP ${CROSS_OBJDUMP} PARENT_SCOPE)
      set(CMAKE_OBJCOPY ${CROSS_OBJCOPY} PARENT_SCOPE)	
      set(CMAKE_SIZE ${CROSS_SIZE} PARENT_SCOPE)		  
      return()
    endif()
  endforeach()
endfunction()

find_toolchain("riscv64-unknown-elf-")

set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")
set(l_L "-L")
set(l_opt ${CROSS_PLATFORM_USER_DIR}/.niiet_aspect/niiet_aspect_sdk/device/K1921VG015/ldscripts CACHE PATH "l_opt")
message(STATUS "l_opt: ${l_opt}")
set(t_T "-T")
set(t_opt ${CROSS_PLATFORM_USER_DIR}/.niiet_aspect/niiet_aspect_sdk/device/K1921VG015/ldscripts/k1921vg015_flash.ld CACHE PATH "t_opt")
message(STATUS "t_opt: ${t_opt}")
add_compile_options(
  -c
  -Wall
  -O0	
  -march=rv32imfc_zicsr
  -mabi=ilp32f
	-g 
	-ggdb3
	-DRETARGET= 
	-DHSECLK_VAL=16000000 
	-DSYSCLK_PLL= 
	"-DCKO_PLL0="
	-static 
	-std=gnu99 
	${l_L}${l_opt}
	-lc_nano 
	-lg_nano 
	-lgcc 
)

add_link_options(
  -static
  -march=rv32imfc_zicsr
  -mabi=ilp32f
	-g 
	-ggdb3
	-std=gnu99 
	-static 
	"${l_L}${l_opt}"  
	-lc_nano 
	-lg_nano 
	-lgcc
  -nostartfiles 
	"${t_T}${t_opt}"
)

set(default_build_type "Debug")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
