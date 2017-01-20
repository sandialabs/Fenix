# Install script for directory: /home/evalen/public/Fenix

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
    set(CMAKE_INSTALL_CONFIG_NAME "")
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
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/evalen/public/Fenix/src/cmake_install.cmake")
  include("/home/evalen/public/Fenix/test/sendrecv/cmake_install.cmake")
  include("/home/evalen/public/Fenix/test/online/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/01_hello_world/fenix/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/01_hello_world/mpi/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/02_send_recv/fenix/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/02_send_recv/mpi/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/03_reduce/fenix/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/03_reduce/mpi/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/04_Isend_Irecv/fenix/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/04_Isend_Irecv/mpi/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/05_subset_create/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/06_subset_createv/cmake_install.cmake")
  include("/home/evalen/public/Fenix/examples/07_subset_full/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/evalen/public/Fenix/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
