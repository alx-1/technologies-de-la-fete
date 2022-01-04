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
  "CMakeFiles/link_tdlf_server.elf.dir/project_elf_src_esp32.c.obj"
  "link_tdlf_server.elf"
  "link_tdlf_server.elf.pdb"
  "project_elf_src_esp32.c"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/link_tdlf_server.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
