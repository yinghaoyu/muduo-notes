# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/d/src/muduo-notes

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/d/src/muduo-notes/build

# Include any dependencies generated for this target.
include tests/CMakeFiles/test_echoclient.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/test_echoclient.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/test_echoclient.dir/flags.make

tests/CMakeFiles/test_echoclient.dir/test_echoclient.cc.o: tests/CMakeFiles/test_echoclient.dir/flags.make
tests/CMakeFiles/test_echoclient.dir/test_echoclient.cc.o: ../tests/test_echoclient.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/d/src/muduo-notes/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/test_echoclient.dir/test_echoclient.cc.o"
	cd /mnt/d/src/muduo-notes/build/tests && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_echoclient.dir/test_echoclient.cc.o -c /mnt/d/src/muduo-notes/tests/test_echoclient.cc

tests/CMakeFiles/test_echoclient.dir/test_echoclient.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_echoclient.dir/test_echoclient.cc.i"
	cd /mnt/d/src/muduo-notes/build/tests && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/d/src/muduo-notes/tests/test_echoclient.cc > CMakeFiles/test_echoclient.dir/test_echoclient.cc.i

tests/CMakeFiles/test_echoclient.dir/test_echoclient.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_echoclient.dir/test_echoclient.cc.s"
	cd /mnt/d/src/muduo-notes/build/tests && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/d/src/muduo-notes/tests/test_echoclient.cc -o CMakeFiles/test_echoclient.dir/test_echoclient.cc.s

# Object files for target test_echoclient
test_echoclient_OBJECTS = \
"CMakeFiles/test_echoclient.dir/test_echoclient.cc.o"

# External object files for target test_echoclient
test_echoclient_EXTERNAL_OBJECTS =

bin/test_echoclient: tests/CMakeFiles/test_echoclient.dir/test_echoclient.cc.o
bin/test_echoclient: tests/CMakeFiles/test_echoclient.dir/build.make
bin/test_echoclient: lib/libmuduo_net.a
bin/test_echoclient: lib/libmuduo_base.a
bin/test_echoclient: tests/CMakeFiles/test_echoclient.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/d/src/muduo-notes/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/test_echoclient"
	cd /mnt/d/src/muduo-notes/build/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_echoclient.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/test_echoclient.dir/build: bin/test_echoclient

.PHONY : tests/CMakeFiles/test_echoclient.dir/build

tests/CMakeFiles/test_echoclient.dir/clean:
	cd /mnt/d/src/muduo-notes/build/tests && $(CMAKE_COMMAND) -P CMakeFiles/test_echoclient.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/test_echoclient.dir/clean

tests/CMakeFiles/test_echoclient.dir/depend:
	cd /mnt/d/src/muduo-notes/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/d/src/muduo-notes /mnt/d/src/muduo-notes/tests /mnt/d/src/muduo-notes/build /mnt/d/src/muduo-notes/build/tests /mnt/d/src/muduo-notes/build/tests/CMakeFiles/test_echoclient.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/test_echoclient.dir/depend

