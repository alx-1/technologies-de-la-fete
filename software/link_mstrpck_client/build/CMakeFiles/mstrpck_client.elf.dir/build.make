# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.18.4/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.18.4/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build

# Include any dependencies generated for this target.
include CMakeFiles/mstrpck_client.elf.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/mstrpck_client.elf.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mstrpck_client.elf.dir/flags.make

project_elf_src.c:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating project_elf_src.c"
	/usr/local/Cellar/cmake/3.18.4/bin/cmake -E touch /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/project_elf_src.c

CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.obj: CMakeFiles/mstrpck_client.elf.dir/flags.make
CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.obj: project_elf_src.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.obj"
	/Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.obj -c /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/project_elf_src.c

CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.i"
	/Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/project_elf_src.c > CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.i

CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.s"
	/Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/project_elf_src.c -o CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.s

# Object files for target mstrpck_client.elf
mstrpck_client_elf_OBJECTS = \
"CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.obj"

# External object files for target mstrpck_client.elf
mstrpck_client_elf_EXTERNAL_OBJECTS =

mstrpck_client.elf: CMakeFiles/mstrpck_client.elf.dir/project_elf_src.c.obj
mstrpck_client.elf: CMakeFiles/mstrpck_client.elf.dir/build.make
mstrpck_client.elf: esp-idf/xtensa/libxtensa.a
mstrpck_client.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
mstrpck_client.elf: esp-idf/app_update/libapp_update.a
mstrpck_client.elf: esp-idf/spi_flash/libspi_flash.a
mstrpck_client.elf: esp-idf/bootloader_support/libbootloader_support.a
mstrpck_client.elf: esp-idf/efuse/libefuse.a
mstrpck_client.elf: esp-idf/driver/libdriver.a
mstrpck_client.elf: esp-idf/nvs_flash/libnvs_flash.a
mstrpck_client.elf: esp-idf/pthread/libpthread.a
mstrpck_client.elf: esp-idf/espcoredump/libespcoredump.a
mstrpck_client.elf: esp-idf/perfmon/libperfmon.a
mstrpck_client.elf: esp-idf/esp32/libesp32.a
mstrpck_client.elf: esp-idf/esp_common/libesp_common.a
mstrpck_client.elf: esp-idf/soc/libsoc.a
mstrpck_client.elf: esp-idf/esp_eth/libesp_eth.a
mstrpck_client.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
mstrpck_client.elf: esp-idf/esp_netif/libesp_netif.a
mstrpck_client.elf: esp-idf/esp_event/libesp_event.a
mstrpck_client.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
mstrpck_client.elf: esp-idf/esp_wifi/libesp_wifi.a
mstrpck_client.elf: esp-idf/lwip/liblwip.a
mstrpck_client.elf: esp-idf/log/liblog.a
mstrpck_client.elf: esp-idf/heap/libheap.a
mstrpck_client.elf: esp-idf/freertos/libfreertos.a
mstrpck_client.elf: esp-idf/vfs/libvfs.a
mstrpck_client.elf: esp-idf/newlib/libnewlib.a
mstrpck_client.elf: esp-idf/cxx/libcxx.a
mstrpck_client.elf: esp-idf/app_trace/libapp_trace.a
mstrpck_client.elf: esp-idf/asio/libasio.a
mstrpck_client.elf: esp-idf/cbor/libcbor.a
mstrpck_client.elf: esp-idf/coap/libcoap.a
mstrpck_client.elf: esp-idf/console/libconsole.a
mstrpck_client.elf: esp-idf/nghttp/libnghttp.a
mstrpck_client.elf: esp-idf/esp-tls/libesp-tls.a
mstrpck_client.elf: esp-idf/esp_adc_cal/libesp_adc_cal.a
mstrpck_client.elf: esp-idf/esp_gdbstub/libesp_gdbstub.a
mstrpck_client.elf: esp-idf/tcp_transport/libtcp_transport.a
mstrpck_client.elf: esp-idf/esp_http_client/libesp_http_client.a
mstrpck_client.elf: esp-idf/esp_http_server/libesp_http_server.a
mstrpck_client.elf: esp-idf/esp_https_ota/libesp_https_ota.a
mstrpck_client.elf: esp-idf/protobuf-c/libprotobuf-c.a
mstrpck_client.elf: esp-idf/protocomm/libprotocomm.a
mstrpck_client.elf: esp-idf/mdns/libmdns.a
mstrpck_client.elf: esp-idf/esp_local_ctrl/libesp_local_ctrl.a
mstrpck_client.elf: esp-idf/sdmmc/libsdmmc.a
mstrpck_client.elf: esp-idf/esp_serial_slave_link/libesp_serial_slave_link.a
mstrpck_client.elf: esp-idf/esp_websocket_client/libesp_websocket_client.a
mstrpck_client.elf: esp-idf/expat/libexpat.a
mstrpck_client.elf: esp-idf/wear_levelling/libwear_levelling.a
mstrpck_client.elf: esp-idf/fatfs/libfatfs.a
mstrpck_client.elf: esp-idf/freemodbus/libfreemodbus.a
mstrpck_client.elf: esp-idf/jsmn/libjsmn.a
mstrpck_client.elf: esp-idf/json/libjson.a
mstrpck_client.elf: esp-idf/libsodium/liblibsodium.a
mstrpck_client.elf: esp-idf/mqtt/libmqtt.a
mstrpck_client.elf: esp-idf/openssl/libopenssl.a
mstrpck_client.elf: esp-idf/spiffs/libspiffs.a
mstrpck_client.elf: esp-idf/ulp/libulp.a
mstrpck_client.elf: esp-idf/unity/libunity.a
mstrpck_client.elf: esp-idf/wifi_provisioning/libwifi_provisioning.a
mstrpck_client.elf: esp-idf/main/libmain.a
mstrpck_client.elf: esp-idf/apa102/libapa102.a
mstrpck_client.elf: esp-idf/protocol_examples_common/libprotocol_examples_common.a
mstrpck_client.elf: esp-idf/asio/libasio.a
mstrpck_client.elf: esp-idf/cbor/libcbor.a
mstrpck_client.elf: esp-idf/coap/libcoap.a
mstrpck_client.elf: esp-idf/esp_adc_cal/libesp_adc_cal.a
mstrpck_client.elf: esp-idf/esp_gdbstub/libesp_gdbstub.a
mstrpck_client.elf: esp-idf/esp_https_ota/libesp_https_ota.a
mstrpck_client.elf: esp-idf/esp_local_ctrl/libesp_local_ctrl.a
mstrpck_client.elf: esp-idf/esp_serial_slave_link/libesp_serial_slave_link.a
mstrpck_client.elf: esp-idf/esp_websocket_client/libesp_websocket_client.a
mstrpck_client.elf: esp-idf/expat/libexpat.a
mstrpck_client.elf: esp-idf/fatfs/libfatfs.a
mstrpck_client.elf: esp-idf/sdmmc/libsdmmc.a
mstrpck_client.elf: esp-idf/wear_levelling/libwear_levelling.a
mstrpck_client.elf: esp-idf/freemodbus/libfreemodbus.a
mstrpck_client.elf: esp-idf/jsmn/libjsmn.a
mstrpck_client.elf: esp-idf/libsodium/liblibsodium.a
mstrpck_client.elf: esp-idf/mqtt/libmqtt.a
mstrpck_client.elf: esp-idf/openssl/libopenssl.a
mstrpck_client.elf: esp-idf/spiffs/libspiffs.a
mstrpck_client.elf: esp-idf/unity/libunity.a
mstrpck_client.elf: esp-idf/wifi_provisioning/libwifi_provisioning.a
mstrpck_client.elf: esp-idf/protocomm/libprotocomm.a
mstrpck_client.elf: esp-idf/protobuf-c/libprotobuf-c.a
mstrpck_client.elf: esp-idf/mdns/libmdns.a
mstrpck_client.elf: esp-idf/console/libconsole.a
mstrpck_client.elf: esp-idf/json/libjson.a
mstrpck_client.elf: esp-idf/xtensa/libxtensa.a
mstrpck_client.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
mstrpck_client.elf: esp-idf/app_update/libapp_update.a
mstrpck_client.elf: esp-idf/spi_flash/libspi_flash.a
mstrpck_client.elf: esp-idf/bootloader_support/libbootloader_support.a
mstrpck_client.elf: esp-idf/efuse/libefuse.a
mstrpck_client.elf: esp-idf/driver/libdriver.a
mstrpck_client.elf: esp-idf/nvs_flash/libnvs_flash.a
mstrpck_client.elf: esp-idf/pthread/libpthread.a
mstrpck_client.elf: esp-idf/espcoredump/libespcoredump.a
mstrpck_client.elf: esp-idf/perfmon/libperfmon.a
mstrpck_client.elf: esp-idf/esp32/libesp32.a
mstrpck_client.elf: esp-idf/esp_common/libesp_common.a
mstrpck_client.elf: esp-idf/soc/libsoc.a
mstrpck_client.elf: esp-idf/esp_eth/libesp_eth.a
mstrpck_client.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
mstrpck_client.elf: esp-idf/esp_netif/libesp_netif.a
mstrpck_client.elf: esp-idf/esp_event/libesp_event.a
mstrpck_client.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
mstrpck_client.elf: esp-idf/esp_wifi/libesp_wifi.a
mstrpck_client.elf: esp-idf/lwip/liblwip.a
mstrpck_client.elf: esp-idf/log/liblog.a
mstrpck_client.elf: esp-idf/heap/libheap.a
mstrpck_client.elf: esp-idf/freertos/libfreertos.a
mstrpck_client.elf: esp-idf/vfs/libvfs.a
mstrpck_client.elf: esp-idf/newlib/libnewlib.a
mstrpck_client.elf: esp-idf/cxx/libcxx.a
mstrpck_client.elf: esp-idf/app_trace/libapp_trace.a
mstrpck_client.elf: esp-idf/nghttp/libnghttp.a
mstrpck_client.elf: esp-idf/esp-tls/libesp-tls.a
mstrpck_client.elf: esp-idf/tcp_transport/libtcp_transport.a
mstrpck_client.elf: esp-idf/esp_http_client/libesp_http_client.a
mstrpck_client.elf: esp-idf/esp_http_server/libesp_http_server.a
mstrpck_client.elf: esp-idf/ulp/libulp.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
mstrpck_client.elf: esp-idf/xtensa/libxtensa.a
mstrpck_client.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
mstrpck_client.elf: esp-idf/app_update/libapp_update.a
mstrpck_client.elf: esp-idf/spi_flash/libspi_flash.a
mstrpck_client.elf: esp-idf/bootloader_support/libbootloader_support.a
mstrpck_client.elf: esp-idf/efuse/libefuse.a
mstrpck_client.elf: esp-idf/driver/libdriver.a
mstrpck_client.elf: esp-idf/nvs_flash/libnvs_flash.a
mstrpck_client.elf: esp-idf/pthread/libpthread.a
mstrpck_client.elf: esp-idf/espcoredump/libespcoredump.a
mstrpck_client.elf: esp-idf/perfmon/libperfmon.a
mstrpck_client.elf: esp-idf/esp32/libesp32.a
mstrpck_client.elf: esp-idf/esp_common/libesp_common.a
mstrpck_client.elf: esp-idf/soc/libsoc.a
mstrpck_client.elf: esp-idf/esp_eth/libesp_eth.a
mstrpck_client.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
mstrpck_client.elf: esp-idf/esp_netif/libesp_netif.a
mstrpck_client.elf: esp-idf/esp_event/libesp_event.a
mstrpck_client.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
mstrpck_client.elf: esp-idf/esp_wifi/libesp_wifi.a
mstrpck_client.elf: esp-idf/lwip/liblwip.a
mstrpck_client.elf: esp-idf/log/liblog.a
mstrpck_client.elf: esp-idf/heap/libheap.a
mstrpck_client.elf: esp-idf/freertos/libfreertos.a
mstrpck_client.elf: esp-idf/vfs/libvfs.a
mstrpck_client.elf: esp-idf/newlib/libnewlib.a
mstrpck_client.elf: esp-idf/cxx/libcxx.a
mstrpck_client.elf: esp-idf/app_trace/libapp_trace.a
mstrpck_client.elf: esp-idf/nghttp/libnghttp.a
mstrpck_client.elf: esp-idf/esp-tls/libesp-tls.a
mstrpck_client.elf: esp-idf/tcp_transport/libtcp_transport.a
mstrpck_client.elf: esp-idf/esp_http_client/libesp_http_client.a
mstrpck_client.elf: esp-idf/esp_http_server/libesp_http_server.a
mstrpck_client.elf: esp-idf/ulp/libulp.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
mstrpck_client.elf: esp-idf/xtensa/libxtensa.a
mstrpck_client.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
mstrpck_client.elf: esp-idf/app_update/libapp_update.a
mstrpck_client.elf: esp-idf/spi_flash/libspi_flash.a
mstrpck_client.elf: esp-idf/bootloader_support/libbootloader_support.a
mstrpck_client.elf: esp-idf/efuse/libefuse.a
mstrpck_client.elf: esp-idf/driver/libdriver.a
mstrpck_client.elf: esp-idf/nvs_flash/libnvs_flash.a
mstrpck_client.elf: esp-idf/pthread/libpthread.a
mstrpck_client.elf: esp-idf/espcoredump/libespcoredump.a
mstrpck_client.elf: esp-idf/perfmon/libperfmon.a
mstrpck_client.elf: esp-idf/esp32/libesp32.a
mstrpck_client.elf: esp-idf/esp_common/libesp_common.a
mstrpck_client.elf: esp-idf/soc/libsoc.a
mstrpck_client.elf: esp-idf/esp_eth/libesp_eth.a
mstrpck_client.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
mstrpck_client.elf: esp-idf/esp_netif/libesp_netif.a
mstrpck_client.elf: esp-idf/esp_event/libesp_event.a
mstrpck_client.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
mstrpck_client.elf: esp-idf/esp_wifi/libesp_wifi.a
mstrpck_client.elf: esp-idf/lwip/liblwip.a
mstrpck_client.elf: esp-idf/log/liblog.a
mstrpck_client.elf: esp-idf/heap/libheap.a
mstrpck_client.elf: esp-idf/freertos/libfreertos.a
mstrpck_client.elf: esp-idf/vfs/libvfs.a
mstrpck_client.elf: esp-idf/newlib/libnewlib.a
mstrpck_client.elf: esp-idf/cxx/libcxx.a
mstrpck_client.elf: esp-idf/app_trace/libapp_trace.a
mstrpck_client.elf: esp-idf/nghttp/libnghttp.a
mstrpck_client.elf: esp-idf/esp-tls/libesp-tls.a
mstrpck_client.elf: esp-idf/tcp_transport/libtcp_transport.a
mstrpck_client.elf: esp-idf/esp_http_client/libesp_http_client.a
mstrpck_client.elf: esp-idf/esp_http_server/libesp_http_server.a
mstrpck_client.elf: esp-idf/ulp/libulp.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
mstrpck_client.elf: esp-idf/xtensa/libxtensa.a
mstrpck_client.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
mstrpck_client.elf: esp-idf/app_update/libapp_update.a
mstrpck_client.elf: esp-idf/spi_flash/libspi_flash.a
mstrpck_client.elf: esp-idf/bootloader_support/libbootloader_support.a
mstrpck_client.elf: esp-idf/efuse/libefuse.a
mstrpck_client.elf: esp-idf/driver/libdriver.a
mstrpck_client.elf: esp-idf/nvs_flash/libnvs_flash.a
mstrpck_client.elf: esp-idf/pthread/libpthread.a
mstrpck_client.elf: esp-idf/espcoredump/libespcoredump.a
mstrpck_client.elf: esp-idf/perfmon/libperfmon.a
mstrpck_client.elf: esp-idf/esp32/libesp32.a
mstrpck_client.elf: esp-idf/esp_common/libesp_common.a
mstrpck_client.elf: esp-idf/soc/libsoc.a
mstrpck_client.elf: esp-idf/esp_eth/libesp_eth.a
mstrpck_client.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
mstrpck_client.elf: esp-idf/esp_netif/libesp_netif.a
mstrpck_client.elf: esp-idf/esp_event/libesp_event.a
mstrpck_client.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
mstrpck_client.elf: esp-idf/esp_wifi/libesp_wifi.a
mstrpck_client.elf: esp-idf/lwip/liblwip.a
mstrpck_client.elf: esp-idf/log/liblog.a
mstrpck_client.elf: esp-idf/heap/libheap.a
mstrpck_client.elf: esp-idf/freertos/libfreertos.a
mstrpck_client.elf: esp-idf/vfs/libvfs.a
mstrpck_client.elf: esp-idf/newlib/libnewlib.a
mstrpck_client.elf: esp-idf/cxx/libcxx.a
mstrpck_client.elf: esp-idf/app_trace/libapp_trace.a
mstrpck_client.elf: esp-idf/nghttp/libnghttp.a
mstrpck_client.elf: esp-idf/esp-tls/libesp-tls.a
mstrpck_client.elf: esp-idf/tcp_transport/libtcp_transport.a
mstrpck_client.elf: esp-idf/esp_http_client/libesp_http_client.a
mstrpck_client.elf: esp-idf/esp_http_server/libesp_http_server.a
mstrpck_client.elf: esp-idf/ulp/libulp.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
mstrpck_client.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/xtensa/esp32/libhal.a
mstrpck_client.elf: esp-idf/newlib/libnewlib.a
mstrpck_client.elf: esp-idf/pthread/libpthread.a
mstrpck_client.elf: esp-idf/app_trace/libapp_trace.a
mstrpck_client.elf: esp-idf/app_trace/libapp_trace.a
mstrpck_client.elf: esp-idf/esp32/esp32_out.ld
mstrpck_client.elf: esp-idf/esp32/ld/esp32.project.ld
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp32/ld/esp32.peripherals.ld
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.newlib-time.ld
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.ld
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.libgcc.ld
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.newlib-data.ld
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.syscalls.ld
mstrpck_client.elf: /Users/alx/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.newlib-funcs.ld
mstrpck_client.elf: CMakeFiles/mstrpck_client.elf.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable mstrpck_client.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mstrpck_client.elf.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mstrpck_client.elf.dir/build: mstrpck_client.elf

.PHONY : CMakeFiles/mstrpck_client.elf.dir/build

CMakeFiles/mstrpck_client.elf.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mstrpck_client.elf.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mstrpck_client.elf.dir/clean

CMakeFiles/mstrpck_client.elf.dir/depend: project_elf_src.c
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/CMakeFiles/mstrpck_client.elf.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mstrpck_client.elf.dir/depend

