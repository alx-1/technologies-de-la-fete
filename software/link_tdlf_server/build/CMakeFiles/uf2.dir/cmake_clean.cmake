file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "flash_project_args"
  "link_tdlf_server.bin"
  "link_tdlf_server.map"
  "project_elf_src_esp32.c"
  "CMakeFiles/uf2"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/uf2.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
