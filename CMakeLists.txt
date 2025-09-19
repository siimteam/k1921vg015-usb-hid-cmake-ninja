cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/riscv64-unknown-elf.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(USB_hid) 

enable_language(C CXX ASM)
set(TARGET_ELF ${PROJECT_NAME}.elf)

include_directories(
        ${CMAKE_SOURCE_DIR}/src 
        ${CMAKE_SOURCE_DIR}/device/Include
)

set(SOURCES "")
file(GLOB_RECURSE C_FILES ${CMAKE_SOURCE_DIR}/*/*.c)
file(GLOB_RECURSE ASM_FILES "*.s" "*.asm")
list(APPEND SOURCES ${C_FILES} ${ASM_FILES})
message(STATUS "SOURCES: ${SOURCES}")

#CMAKE CACHE main() function LIFEHACK
file(GLOB_RECURSE REMOVE_CMAKE "${CMAKE_SOURCE_DIR}/build/*")
list(REMOVE_ITEM SOURCES ${REMOVE_CMAKE})
#EOLIFEHACK

add_executable(${TARGET_ELF} ${SOURCES})

# add_custom_command(
#    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/romfs.c
#    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/wch/rtthread/tools/mkromfs.py ${RT_DFS_ROMFS_PATH} ${CMAKE_CURRENT_BINARY_DIR}/romfs.c)
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_OBJDUMP} "-CdSx" $<TARGET_FILE:${TARGET_ELF}> > ${PROJECT_NAME}.lst
    COMMENT "Generating disassembler file")
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_OBJCOPY} "-Obinary" $<TARGET_FILE:${TARGET_ELF}> ${PROJECT_NAME}.bin
    COMMENT "Generating bin file")
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_OBJCOPY} "-Oihex" $<TARGET_FILE:${TARGET_ELF}> ${PROJECT_NAME}.hex
    COMMENT "Generating hex file")
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_SIZE} "-A" $<TARGET_FILE:${TARGET_ELF}>
    COMMENT "Output size of file")

#add_custom_target(flash
#        COMMAND openocd -f ${CMAKE_CURRENT_SOURCE_DIR}/wch/wch-riscv.cfg
#            -c init
#            -c halt
#            -c "flash erase_sector wch_riscv 0 last"
#            -c "program ${TARGET_ELF} verify"
#            -c wlink_reset_resume
#            -c exit
#        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#        DEPENDS ${tgt}
#        COMMENT "Running openocd ${tgt_name}"
#        USES_TERMINAL)
