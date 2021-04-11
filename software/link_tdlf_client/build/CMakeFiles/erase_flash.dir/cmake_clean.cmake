file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "link_tdlf_client.bin"
  "link_tdlf_client.map"
  "project_elf_src.c"
  "CMakeFiles/erase_flash"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/erase_flash.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
