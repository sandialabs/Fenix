set(CMAKE_C_COMPILER "/home/evalen/devtools/bin/mpicc")
set(CMAKE_C_COMPILER_ARG1 "")
set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_C_COMPILER_VERSION "5.2.0")
set(CMAKE_C_STANDARD_COMPUTED_DEFAULT "11")
set(CMAKE_C_COMPILE_FEATURES "c_function_prototypes;c_restrict;c_variadic_macros;c_static_assert")
set(CMAKE_C90_COMPILE_FEATURES "c_function_prototypes")
set(CMAKE_C99_COMPILE_FEATURES "c_restrict;c_variadic_macros")
set(CMAKE_C11_COMPILE_FEATURES "c_static_assert")

set(CMAKE_C_PLATFORM_ID "Linux")
set(CMAKE_C_SIMULATE_ID "")
set(CMAKE_C_SIMULATE_VERSION "")

set(CMAKE_AR "/home/projects/x86-64-haswell/bintuils/2.25.0/bin/ar")
set(CMAKE_RANLIB "/home/projects/x86-64-haswell/bintuils/2.25.0/bin/ranlib")
set(CMAKE_LINKER "/home/projects/x86-64-haswell/bintuils/2.25.0/bin/ld")
set(CMAKE_COMPILER_IS_GNUCC 1)
set(CMAKE_C_COMPILER_LOADED 1)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_C_ABI_COMPILED TRUE)
set(CMAKE_COMPILER_IS_MINGW )
set(CMAKE_COMPILER_IS_CYGWIN )
if(CMAKE_COMPILER_IS_CYGWIN)
  set(CYGWIN 1)
  set(UNIX 1)
endif()

set(CMAKE_C_COMPILER_ENV_VAR "CC")

if(CMAKE_COMPILER_IS_MINGW)
  set(MINGW 1)
endif()
set(CMAKE_C_COMPILER_ID_RUN 1)
set(CMAKE_C_SOURCE_FILE_EXTENSIONS c;m)
set(CMAKE_C_IGNORE_EXTENSIONS h;H;o;O;obj;OBJ;def;DEF;rc;RC)
set(CMAKE_C_LINKER_PREFERENCE 10)

# Save compiler ABI information.
set(CMAKE_C_SIZEOF_DATA_PTR "8")
set(CMAKE_C_COMPILER_ABI "ELF")
set(CMAKE_C_LIBRARY_ARCHITECTURE "")

if(CMAKE_C_SIZEOF_DATA_PTR)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_C_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_C_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_C_COMPILER_ABI}")
endif()

if(CMAKE_C_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()

set(CMAKE_C_CL_SHOWINCLUDES_PREFIX "")
if(CMAKE_C_CL_SHOWINCLUDES_PREFIX)
  set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_C_CL_SHOWINCLUDES_PREFIX}")
endif()




set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "mpi;dl;m;numa;pci;pmi;rt;nsl;util;m;dl;pthread;c")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "/home/evalen/devtools/lib;/lib64;/home/projects/x86-64-haswell/gnu/5.2.0/lib64;/usr/lib64;/home/projects/x86-64-haswell/util-linux/20151026/lib;/home/projects/x86-64-haswell/curl/7.48.0/lib;/home/projects/x86-64-haswell/boost/1.59.0/openmpi/1.10.2/intel/17.0.042/lib;/home/projects/x86-64-haswell/python/2.7.9/lib;/home/projects/x86-64-haswell/valgrind/3.11.0/lib;/home/projects/x86-64-haswell/gnu/5.2.0/lib/gcc/x86_64-unknown-linux-gnu/5.2.0;/home/projects/x86-64-haswell/gnu/5.2.0/lib;/home/projects/x86-64-haswell/bintuils/2.25.0/lib;/home/projects/x86-64-haswell/zlib/1.2.8/lib;/home/projects/x86-64-haswell/gmp/5.1.3/lib;/home/projects/x86-64-haswell/mpfr/3.1.2/lib;/home/projects/x86-64-haswell/mpc/1.0.1/lib;/usr/lib/gcc/x86_64-redhat-linux/4.4.7;/home/projects/x86-64-haswell/bintuils/2.25.0/x86_64-unknown-linux-gnu/lib")
set(CMAKE_C_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "")
