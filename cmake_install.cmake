# Install script for directory: /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/src/cmake_install.cmake")
  include("/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/applis/eperftool/cmake_install.cmake")
  include("/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/applis/howto_examples/simple_client_server/cmake_install.cmake")
  include("/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tools/descr_stats_v1.2/cmake_install.cmake")
  include("/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/tests/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
