#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

fenix_test(SRC replay_test.cpp RANKS 6 EXE replay_test_pt2pt MLOG)

fenix_test(SRC replay_test.cpp RANKS 6 EXE replay_test_barriers MLOG
   DEFINITIONS FENIX_STENCIL_ENABLE_BARRIERS)

fenix_test(SRC replay_test.cpp RANKS 6 EXE replay_test_bcasts MLOG
   DEFINITIONS FENIX_STENCIL_ENABLE_BCASTS)

fenix_test(SRC replay_test.cpp RANKS 6 EXE replay_test_reduces MLOG
   DEFINITIONS FENIX_STENCIL_ENABLE_REDUCES)

fenix_test(SRC replay_test.cpp RANKS 6 EXE replay_test_allreduces MLOG
   DEFINITIONS FENIX_STENCIL_ENABLE_ALLREDUCES)

fenix_test(SRC replay_test.cpp RANKS 6 EXE replay_test_everything MLOG
   DEFINITIONS FENIX_STENCIL_ENABLE_BARRIERS
               FENIX_STENCIL_ENABLE_BCASTS
               FENIX_STENCIL_ENABLE_REDUCES
               FENIX_STENCIL_ENABLE_ALLREDUCES)
