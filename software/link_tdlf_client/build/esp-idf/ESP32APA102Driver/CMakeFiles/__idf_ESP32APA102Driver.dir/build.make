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
CMAKE_SOURCE_DIR = /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build

# Include any dependencies generated for this target.
include esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/depend.make

# Include the progress variables for this target.
include esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/progress.make

# Include the compile flags for this target's objects.
include esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/flags.make

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.obj: esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/flags.make
esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.obj: ../components/ESP32APA102Driver/apa102LEDStrip.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.obj"
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.obj -c /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/components/ESP32APA102Driver/apa102LEDStrip.c

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.i"
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/components/ESP32APA102Driver/apa102LEDStrip.c > CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.i

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.s"
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/components/ESP32APA102Driver/apa102LEDStrip.c -o CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.s

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.obj: esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/flags.make
esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.obj: ../components/ESP32APA102Driver/colourObject.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.obj"
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.obj -c /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/components/ESP32APA102Driver/colourObject.c

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.i"
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/components/ESP32APA102Driver/colourObject.c > CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.i

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.s"
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && /Users/alx/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/components/ESP32APA102Driver/colourObject.c -o CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.s

# Object files for target __idf_ESP32APA102Driver
__idf_ESP32APA102Driver_OBJECTS = \
"CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.obj" \
"CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.obj"

# External object files for target __idf_ESP32APA102Driver
__idf_ESP32APA102Driver_EXTERNAL_OBJECTS =

esp-idf/ESP32APA102Driver/libESP32APA102Driver.a: esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/apa102LEDStrip.c.obj
esp-idf/ESP32APA102Driver/libESP32APA102Driver.a: esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/colourObject.c.obj
esp-idf/ESP32APA102Driver/libESP32APA102Driver.a: esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/build.make
esp-idf/ESP32APA102Driver/libESP32APA102Driver.a: esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libESP32APA102Driver.a"
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && $(CMAKE_COMMAND) -P CMakeFiles/__idf_ESP32APA102Driver.dir/cmake_clean_target.cmake
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/__idf_ESP32APA102Driver.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/build: esp-idf/ESP32APA102Driver/libESP32APA102Driver.a

.PHONY : esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/build

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/clean:
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver && $(CMAKE_COMMAND) -P CMakeFiles/__idf_ESP32APA102Driver.dir/cmake_clean.cmake
.PHONY : esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/clean

esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/depend:
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/components/ESP32APA102Driver /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : esp-idf/ESP32APA102Driver/CMakeFiles/__idf_ESP32APA102Driver.dir/depend

