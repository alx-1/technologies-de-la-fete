file(REMOVE_RECURSE
  "bootloader.bin"
  "bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "project_elf_src.c"
  "CMakeFiles/encrypted-app-flash"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/encrypted-app-flash.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
