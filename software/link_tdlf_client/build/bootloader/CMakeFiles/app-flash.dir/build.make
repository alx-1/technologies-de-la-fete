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
CMAKE_SOURCE_DIR = /Users/alx/esp/esp-idf/components/bootloader/subproject

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/bootloader

# Utility rule file for app-flash.

# Include the progress variables for this target.
include CMakeFiles/app-flash.dir/progress.make

CMakeFiles/app-flash:
	cd /Users/alx/esp/esp-idf/components/esptool_py && /usr/local/Cellar/cmake/3.18.4/bin/cmake -D IDF_PATH="/Users/alx/esp/esp-idf" -D ESPTOOLPY="/Users/alx/.espressif/python_env/idf4.1_py2.7_env/bin/python /Users/alx/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32" -D ESPTOOL_ARGS="write_flash @flash_app_args" -D WORKING_DIRECTORY="/Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/bootloader" -P /Users/alx/esp/esp-idf/components/esptool_py/run_esptool.cmake

app-flash: CMakeFiles/app-flash
app-flash: CMakeFiles/app-flash.dir/build.make

.PHONY : app-flash

# Rule to build all files generated by this target.
CMakeFiles/app-flash.dir/build: app-flash

.PHONY : CMakeFiles/app-flash.dir/build

CMakeFiles/app-flash.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/app-flash.dir/cmake_clean.cmake
.PHONY : CMakeFiles/app-flash.dir/clean

CMakeFiles/app-flash.dir/depend:
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/bootloader && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/alx/esp/esp-idf/components/bootloader/subproject /Users/alx/esp/esp-idf/components/bootloader/subproject /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/bootloader /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/bootloader /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/bootloader/CMakeFiles/app-flash.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/app-flash.dir/depend
