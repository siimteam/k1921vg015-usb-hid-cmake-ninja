set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)
set(CMAKE_CXX_COMPILER_ID GNU)
set(VENDOR unknown-)
set(MODE elf)

if(WIN32)
    set(CROSS_PLATFORM_USER_DIR "$ENV{USERPROFILE}")
	set(TOOLCHAIN_NAME "riscv64-unknown-elf-")
else()
    set(CROSS_PLATFORM_USER_DIR "$ENV{HOME}")
	set(TOOLCHAIN_NAME "riscv-none-elf-")
endif()

set(CROSSTOOL_PATH ${CROSS_PLATFORM_USER_DIR}/.niiet_aspect/sc-dt-2025.06/riscv-gcc/bin CACHE PATH "Cross toolchain root directory")

#"-L"
set(LIBS_PATH ${CROSSTOOL_PATH}/../riscv64-unknown-elf/lib/rv32imfc_zicsr/ilp32f CACHE PATH "Cross toolchain libs directory")
set(ADD_LIBS_PATH ${CROSSTOOL_PATH}/../lib/gcc/riscv64-unknown-elf/14.2.0/rv32imfc_zicsr/ilp32f CACHE PATH "Cross toolchain addlibs directory")
#link_directories(
#        ${LIBS_PATH}
#        ${ADD_LIBS_PATH}
#)

# Look for GCC in path
function(find_toolchain)
  foreach(CROSS_PREFIX "${ARGV}")
    find_program(CROSS_CC "${CROSS_PREFIX}gcc" "${CROSSTOOL_PATH}" NO_DEFAULT_PATH)
    find_program(CROSS_CXX "${CROSS_PREFIX}g++" "${CROSSTOOL_PATH}" NO_DEFAULT_PATH)
    find_program(CROSS_ASM "${CROSS_PREFIX}as" "${CROSSTOOL_PATH}" NO_DEFAULT_PATH)
    find_program(CROSS_LD "${CROSS_PREFIX}ld" "${CROSSTOOL_PATH}" NO_DEFAULT_PATH)
    find_program(CROSS_OBJDUMP "${CROSS_PREFIX}objdump" "${CROSSTOOL_PATH}" NO_DEFAULT_PATH)
    find_program(CROSS_OBJCOPY "${CROSS_PREFIX}objcopy" "${CROSSTOOL_PATH}" NO_DEFAULT_PATH)
    find_program(CROSS_SIZE "${CROSS_PREFIX}size" "${CROSSTOOL_PATH}" NO_DEFAULT_PATH)
    if (CROSS_CC AND CROSS_ASM AND CROSS_LD AND CROSS_OBJCOPY AND CROSS_OBJCOPY AND CROSS_SIZE)
      message("@@ found toolchain ${CROSS_CC}")
      set(CMAKE_C_COMPILER ${CROSS_CC} PARENT_SCOPE)
      message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
      set(CMAKE_CXX_COMPILER ${CROSS_CXX} PARENT_SCOPE)
      set(CMAKE_ASM_COMPILER ${CROSS_CC} PARENT_SCOPE)
      #set(CMAKE_C_LINK_EXECUTABLE ${CROSS_LD} PARENT_SCOPE)
      #set(CMAKE_CXX_LINK_EXECUTABLE ${CROSS_LD} PARENT_SCOPE)
      #set(CMAKE_ASM_LINK_EXECUTABLE ${CROSS_LD} PARENT_SCOPE)
      set(CMAKE_OBJDUMP ${CROSS_OBJDUMP} PARENT_SCOPE)
      set(CMAKE_OBJCOPY ${CROSS_OBJCOPY} PARENT_SCOPE)	
      set(CMAKE_SIZE ${CROSS_SIZE} PARENT_SCOPE)		  
      return()
    endif()
  endforeach()
endfunction()

find_toolchain(${TOOLCHAIN_NAME})

set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")
#set(CMAKE_ASM_FLAGS "-march=rv32imfc_zicsr -mno-arch-attr")


add_compile_options(
	-c
	-Wall
	-O0	
	-march=rv32imfc_zicsr
	-mabi=ilp32f
	-g 
	-ggdb3
	-static 
	-std=gnu99 
	-lc_nano 
	-lg_nano 
	-lgcc 
)

#  "${l_L}${l_opt}" 
#  "${l_L}${LIBS_PATH}"
#  "${l_L}${ADD_LIBS_PATH}"
#	"${t_T}${t_opt}"
# -nostartfiles
# -nostdlib or -nodefaultlibs

#add_link_options(
#	-o
#	-static
#	-march=rv32imfc_zicsr
#	-melf32lriscv
#	-lc_nano 
#	-lg_nano 
#	-lgcc
#	-nostartfiles
#)
add_link_options(
  -static
  -march=rv32imfc_zicsr
  -mabi=ilp32f
	-g 
	-ggdb3
	-std=gnu99 
	-static 
	-lc_nano 
	-lg_nano 
	-lgcc
	-nostartfiles
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
