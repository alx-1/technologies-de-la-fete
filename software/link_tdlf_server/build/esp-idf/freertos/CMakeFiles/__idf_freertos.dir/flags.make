# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# compile ASM with /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2021r2-8.4.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc
# compile C with /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2021r2-8.4.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc
ASM_DEFINES = 

ASM_INCLUDES = -I/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_server/build/config -I/Users/alx/esp/esp-idf/components/freertos/include -I/Users/alx/esp/esp-idf/components/freertos/port/xtensa/include -I/Users/alx/esp/esp-idf/components/freertos/include/freertos -I/Users/alx/esp/esp-idf/components/freertos/port/xtensa/include/freertos -I/Users/alx/esp/esp-idf/components/freertos/port/xtensa -I/Users/alx/esp/esp-idf/components/freertos -I/Users/alx/esp/esp-idf/components/newlib/platform_include -I/Users/alx/esp/esp-idf/components/esp_hw_support/include -I/Users/alx/esp/esp-idf/components/esp_hw_support/port/esp32/. -I/Users/alx/esp/esp-idf/components/heap/include -I/Users/alx/esp/esp-idf/components/log/include -I/Users/alx/esp/esp-idf/components/lwip/include/apps -I/Users/alx/esp/esp-idf/components/lwip/include/apps/sntp -I/Users/alx/esp/esp-idf/components/lwip/lwip/src/include -I/Users/alx/esp/esp-idf/components/lwip/port/esp32/include -I/Users/alx/esp/esp-idf/components/lwip/port/esp32/include/arch -I/Users/alx/esp/esp-idf/components/soc/include -I/Users/alx/esp/esp-idf/components/soc/esp32/. -I/Users/alx/esp/esp-idf/components/soc/esp32/include -I/Users/alx/esp/esp-idf/components/hal/esp32/include -I/Users/alx/esp/esp-idf/components/hal/include -I/Users/alx/esp/esp-idf/components/esp_rom/include -I/Users/alx/esp/esp-idf/components/esp_rom/esp32 -I/Users/alx/esp/esp-idf/components/esp_rom/include/esp32 -I/Users/alx/esp/esp-idf/components/esp_common/include -I/Users/alx/esp/esp-idf/components/esp_system/include -I/Users/alx/esp/esp-idf/components/esp32/include -I/Users/alx/esp/esp-idf/components/driver/include -I/Users/alx/esp/esp-idf/components/driver/esp32/include -I/Users/alx/esp/esp-idf/components/esp_ringbuf/include -I/Users/alx/esp/esp-idf/components/efuse/include -I/Users/alx/esp/esp-idf/components/efuse/esp32/include -I/Users/alx/esp/esp-idf/components/xtensa/include -I/Users/alx/esp/esp-idf/components/xtensa/esp32/include -I/Users/alx/esp/esp-idf/components/espcoredump/include -I/Users/alx/esp/esp-idf/components/esp_timer/include -I/Users/alx/esp/esp-idf/components/esp_ipc/include -I/Users/alx/esp/esp-idf/components/esp_pm/include -I/Users/alx/esp/esp-idf/components/vfs/include -I/Users/alx/esp/esp-idf/components/esp_wifi/include -I/Users/alx/esp/esp-idf/components/esp_wifi/esp32/include -I/Users/alx/esp/esp-idf/components/esp_event/include -I/Users/alx/esp/esp-idf/components/esp_netif/include -I/Users/alx/esp/esp-idf/components/esp_eth/include -I/Users/alx/esp/esp-idf/components/tcpip_adapter/include -I/Users/alx/esp/esp-idf/components/app_trace/include

ASM_FLAGS = -ffunction-sections -fdata-sections -Wall -Werror=all -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -ggdb -Og -fstrict-volatile-bitfields -Wno-error=unused-but-set-variable -D_GNU_SOURCE -DIDF_VER=\"v4.3.2-dirty\" -DESP_PLATFORM

C_DEFINES = 

C_INCLUDES = -I/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_server/build/config -I/Users/alx/esp/esp-idf/components/freertos/include -I/Users/alx/esp/esp-idf/components/freertos/port/xtensa/include -I/Users/alx/esp/esp-idf/components/freertos/include/freertos -I/Users/alx/esp/esp-idf/components/freertos/port/xtensa/include/freertos -I/Users/alx/esp/esp-idf/components/freertos/port/xtensa -I/Users/alx/esp/esp-idf/components/freertos -I/Users/alx/esp/esp-idf/components/newlib/platform_include -I/Users/alx/esp/esp-idf/components/esp_hw_support/include -I/Users/alx/esp/esp-idf/components/esp_hw_support/port/esp32/. -I/Users/alx/esp/esp-idf/components/heap/include -I/Users/alx/esp/esp-idf/components/log/include -I/Users/alx/esp/esp-idf/components/lwip/include/apps -I/Users/alx/esp/esp-idf/components/lwip/include/apps/sntp -I/Users/alx/esp/esp-idf/components/lwip/lwip/src/include -I/Users/alx/esp/esp-idf/components/lwip/port/esp32/include -I/Users/alx/esp/esp-idf/components/lwip/port/esp32/include/arch -I/Users/alx/esp/esp-idf/components/soc/include -I/Users/alx/esp/esp-idf/components/soc/esp32/. -I/Users/alx/esp/esp-idf/components/soc/esp32/include -I/Users/alx/esp/esp-idf/components/hal/esp32/include -I/Users/alx/esp/esp-idf/components/hal/include -I/Users/alx/esp/esp-idf/components/esp_rom/include -I/Users/alx/esp/esp-idf/components/esp_rom/esp32 -I/Users/alx/esp/esp-idf/components/esp_rom/include/esp32 -I/Users/alx/esp/esp-idf/components/esp_common/include -I/Users/alx/esp/esp-idf/components/esp_system/include -I/Users/alx/esp/esp-idf/components/esp32/include -I/Users/alx/esp/esp-idf/components/driver/include -I/Users/alx/esp/esp-idf/components/driver/esp32/include -I/Users/alx/esp/esp-idf/components/esp_ringbuf/include -I/Users/alx/esp/esp-idf/components/efuse/include -I/Users/alx/esp/esp-idf/components/efuse/esp32/include -I/Users/alx/esp/esp-idf/components/xtensa/include -I/Users/alx/esp/esp-idf/components/xtensa/esp32/include -I/Users/alx/esp/esp-idf/components/espcoredump/include -I/Users/alx/esp/esp-idf/components/esp_timer/include -I/Users/alx/esp/esp-idf/components/esp_ipc/include -I/Users/alx/esp/esp-idf/components/esp_pm/include -I/Users/alx/esp/esp-idf/components/vfs/include -I/Users/alx/esp/esp-idf/components/esp_wifi/include -I/Users/alx/esp/esp-idf/components/esp_wifi/esp32/include -I/Users/alx/esp/esp-idf/components/esp_event/include -I/Users/alx/esp/esp-idf/components/esp_netif/include -I/Users/alx/esp/esp-idf/components/esp_eth/include -I/Users/alx/esp/esp-idf/components/tcpip_adapter/include -I/Users/alx/esp/esp-idf/components/app_trace/include

C_FLAGS = -mlongcalls -Wno-frame-address -ffunction-sections -fdata-sections -Wall -Werror=all -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -ggdb -Og -fstrict-volatile-bitfields -Wno-error=unused-but-set-variable -std=gnu99 -Wno-old-style-declaration -D_GNU_SOURCE -DIDF_VER=\"v4.3.2-dirty\" -DESP_PLATFORM

# Custom defines: esp-idf/freertos/CMakeFiles/__idf_freertos.dir/event_groups.c.obj_DEFINES = _ESP_FREERTOS_INTERNAL

# Custom defines: esp-idf/freertos/CMakeFiles/__idf_freertos.dir/queue.c.obj_DEFINES = _ESP_FREERTOS_INTERNAL

# Custom defines: esp-idf/freertos/CMakeFiles/__idf_freertos.dir/tasks.c.obj_DEFINES = _ESP_FREERTOS_INTERNAL

# Custom defines: esp-idf/freertos/CMakeFiles/__idf_freertos.dir/timers.c.obj_DEFINES = _ESP_FREERTOS_INTERNAL

# Custom defines: esp-idf/freertos/CMakeFiles/__idf_freertos.dir/stream_buffer.c.obj_DEFINES = _ESP_FREERTOS_INTERNAL

