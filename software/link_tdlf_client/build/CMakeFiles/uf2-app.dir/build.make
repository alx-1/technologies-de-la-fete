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

# Utility rule file for uf2-app.

# Include the progress variables for this target.
include CMakeFiles/uf2-app.dir/progress.make

CMakeFiles/uf2-app:
	/Users/alx/.espressif/python_env/idf4.3_py3.9_env/bin/python /Users/alx/esp/esp-idf/tools/mkuf2.py write -o /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/uf2-app.bin --json /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/flasher_args.json --chip-id 0x1c5f21b0 --bin app

uf2-app: CMakeFiles/uf2-app
uf2-app: CMakeFiles/uf2-app.dir/build.make

.PHONY : uf2-app

# Rule to build all files generated by this target.
CMakeFiles/uf2-app.dir/build: uf2-app

.PHONY : CMakeFiles/uf2-app.dir/build

CMakeFiles/uf2-app.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/uf2-app.dir/cmake_clean.cmake
.PHONY : CMakeFiles/uf2-app.dir/clean

CMakeFiles/uf2-app.dir/depend:
	cd /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build /Users/alx/Documents/GitHub/technologies-de-la-fete/software/link_tdlf_client/build/CMakeFiles/uf2-app.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/uf2-app.dir/depend

