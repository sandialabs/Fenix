#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and  Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

target_sources(fenix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/message_logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/comm_log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rank_log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/msg_log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/task.cpp
)

target_sources(mlog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mpi_overload.cpp
)
