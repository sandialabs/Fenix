#
#  This file is part of Fenix
#  Copyright (c) 2016 Rutgers University and Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the LICENSE file in the top Fenix
#  directory.
#

function(subset_test name)
  set(exe fenix_subset_${name})
  add_executable(${exe} subset_${name}.cpp)
  target_link_libraries(${exe} fenix)

  add_test(subset_${name} ${exe})
endfunction()

subset_test(includes)
subset_test(subtraction)
subset_test(copy)
subset_test(addition)
subset_test(equality)
subset_test(serialize_data)
subset_test(serialize)
