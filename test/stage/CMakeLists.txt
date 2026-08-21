#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#


fenix_test(SRC stage.cpp RANKS 6)
fenix_test(SRC stage_dynamic_file.cpp RANKS 6)
fenix_test(SRC stage_dynamic_stream.cpp RANKS 6)
fenix_test(SRC stage_begin_file.cpp RANKS 6)
fenix_test(SRC stage_begin_stream.cpp RANKS 6)
fenix_test(SRC stage_seek.cpp RANKS 2)
