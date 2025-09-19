USB_HID_NIIET_CMAKE
|   
|   CMakeLists.txt
|   makefile
|   niiet_aspect.json
|       
+---cmake
|       riscv64-unknown-elf.cmake
|       
+---device
|   |   config.json
|   |   
|   +---Include
|   |       arch.h
|   |       csr.h
|   |       div_macros.h
|   |       K1921VG015.h
|   |       memasm.h
|   |       plic.h
|   |       retarget.h
|   |       stringify.h
|   |       system_k1921vg015.h
|   |       
|   +---ldscripts
|   |       k1921vg015_common.lds
|   |       k1921vg015_flash.ld
|   |       k1921vg015_ram.ld
|   |       
|   \---Source
|           plic.c
|           printf.c
|           retarget.c
|           startup_k1921vg015.S
|           system_k1921vg015.c
|           sys_init.c
|           
\---src
        main.c
        usbhid.c
        usbhid.h
        
