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
include tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/depend.make

# Include the progress variables for this target.
include tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/progress.make

# Include the compile flags for this target's objects.
include tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/flags.make

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o: tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/flags.make
tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o: tools/descr_stats_v1.2/descr_stats.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2 && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/descr_stats.dir/descr_stats.c.o   -c /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2/descr_stats.c

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/descr_stats.dir/descr_stats.c.i"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2 && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2/descr_stats.c > CMakeFiles/descr_stats.dir/descr_stats.c.i

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/descr_stats.dir/descr_stats.c.s"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2 && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2/descr_stats.c -o CMakeFiles/descr_stats.dir/descr_stats.c.s

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.requires:

.PHONY : tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.requires

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.provides: tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.requires
	$(MAKE) -f tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/build.make tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.provides.build
.PHONY : tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.provides

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.provides.build: tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o


# Object files for target descr_stats
descr_stats_OBJECTS = \
"CMakeFiles/descr_stats.dir/descr_stats.c.o"

# External object files for target descr_stats
descr_stats_EXTERNAL_OBJECTS =

perf_eval/descr_stats: tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o
perf_eval/descr_stats: tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/build.make
perf_eval/descr_stats: tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable ../../perf_eval/descr_stats"
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2 && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/descr_stats.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/build: perf_eval/descr_stats

.PHONY : tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/build

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/requires: tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/descr_stats.c.o.requires

.PHONY : tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/requires

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/clean:
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2 && $(CMAKE_COMMAND) -P CMakeFiles/descr_stats.dir/cmake_clean.cmake
.PHONY : tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/clean

tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/depend:
	cd /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2 /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2 /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2 /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2 /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools/descr_stats_v1.2/CMakeFiles/descr_stats.dir/depend

