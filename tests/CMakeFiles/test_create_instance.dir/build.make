# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

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
CMAKE_SOURCE_DIR = /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2

# Include any dependencies generated for this target.
include tests/CMakeFiles/test_create_instance.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/test_create_instance.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/test_create_instance.dir/flags.make

tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o: tests/CMakeFiles/test_create_instance.dir/flags.make
tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o: tests/create_instance_test.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/test_create_instance.dir/create_instance_test.c.o   -c /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests/create_instance_test.c

tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_create_instance.dir/create_instance_test.c.i"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests/create_instance_test.c > CMakeFiles/test_create_instance.dir/create_instance_test.c.i

tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_create_instance.dir/create_instance_test.c.s"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests/create_instance_test.c -o CMakeFiles/test_create_instance.dir/create_instance_test.c.s

tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.requires:

.PHONY : tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.requires

tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.provides: tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.requires
	$(MAKE) -f tests/CMakeFiles/test_create_instance.dir/build.make tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.provides.build
.PHONY : tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.provides

tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.provides.build: tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o


# Object files for target test_create_instance
test_create_instance_OBJECTS = \
"CMakeFiles/test_create_instance.dir/create_instance_test.c.o"

# External object files for target test_create_instance
test_create_instance_EXTERNAL_OBJECTS =

bin/Release/test_create_instance: tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o
bin/Release/test_create_instance: tests/CMakeFiles/test_create_instance.dir/build.make
bin/Release/test_create_instance: bin/Release/libopenfec.so.1.4.2
bin/Release/test_create_instance: tests/CMakeFiles/test_create_instance.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable ../bin/Release/test_create_instance"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_create_instance.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/test_create_instance.dir/build: bin/Release/test_create_instance

.PHONY : tests/CMakeFiles/test_create_instance.dir/build

tests/CMakeFiles/test_create_instance.dir/requires: tests/CMakeFiles/test_create_instance.dir/create_instance_test.c.o.requires

.PHONY : tests/CMakeFiles/test_create_instance.dir/requires

tests/CMakeFiles/test_create_instance.dir/clean:
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests && $(CMAKE_COMMAND) -P CMakeFiles/test_create_instance.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/test_create_instance.dir/clean

tests/CMakeFiles/test_create_instance.dir/depend:
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2 /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2 /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests/CMakeFiles/test_create_instance.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/test_create_instance.dir/depend

