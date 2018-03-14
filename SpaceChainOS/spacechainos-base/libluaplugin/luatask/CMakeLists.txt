# Copyright (C) 2007-2012 LuaDist.
# Created by Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

project ( luatask C )
cmake_minimum_required ( VERSION 2.8 )
include ( cmake/dist.cmake )
include ( lua )

# Detect threading support.
# TODO: Windows can use Pthreads-Win32.
find_package ( Threads )

if ( CMAKE_USE_WIN32_THREADS_INIT )
  add_definitions ( -DNATV_WIN32 "-DLUATASK_API=__declspec(dllexport)" )
elseif ( CMAKE_USE_PTHREADS_INIT )
  add_definitions ( -DLUATASK_API= -DLUATASK_PTHREAD_STACK_SIZE=2097152/16 )
  set ( LIBS pthread )
endif ( )

# Build modules
install_lua_module ( task src/LuaTask/ltask.c src/LuaTask/queue.c src/LuaTask/syncos.c LINK ${LIBS} )

# Install all files and documentation
install_data ( LICENSE README )
install_doc ( doc/ )
install_example ( examples/ )
