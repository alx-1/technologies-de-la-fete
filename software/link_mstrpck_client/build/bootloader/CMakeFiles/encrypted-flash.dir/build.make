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
CMAKE_BINARY_DIR = /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/bootloader

# Utility rule file for encrypted-flash.

# Include the progress variables for this target.
include CMakeFiles/encrypted-flash.dir/progress.make

CMakeFiles/encrypted-flash:
	/usr/local/Cellar/cmake/3.18.4/bin/cmake -E echo "Error: The target encrypted-flash requires"
	/usr/local/Cellar/cmake/3.18.4/bin/cmake -E echo "CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT to be enabled."
	/usr/local/Cellar/cmake/3.18.4/bin/cmake -E env "FAIL_MESSAGE=Failed executing target (see errors on lines above)" /usr/local/Cellar/cmake/3.18.4/bin/cmake -P /Users/alx/esp/esp-idf/tools/cmake/scripts/fail.cmake

encrypted-flash: CMakeFiles/encrypted-flash
encrypted-flash: CMakeFiles/encrypted-flash.dir/build.make

.PHONY : encrypted-flash

# Rule to build all files generated by this target.
CMakeFiles/encrypted-flash.dir/build: encrypted-flash

.PHONY : CMakeFiles/encrypted-flash.dir/build

CMakeFiles/encrypted-flash.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/encrypted-flash.dir/cmake_clean.cmake
.PHONY : CMakeFiles/encrypted-flash.dir/clean

CMakeFiles/encrypted-flash.dir/depend:
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/bootloader && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/alx/esp/esp-idf/components/bootloader/subproject /Users/alx/esp/esp-idf/components/bootloader/subproject /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/bootloader /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/bootloader /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_mstrpck_client/build/bootloader/CMakeFiles/encrypted-flash.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/encrypted-flash.dir/depend

