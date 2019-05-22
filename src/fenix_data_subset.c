/*
//@HEADER
// ************************************************************************
//
//
//            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
//            _|        _|        _|_|    _|    _|      _|  _|
//            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
//            _|        _|        _|    _|_|    _|      _|  _|
//            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|
//
//
//
//
// Copyright (C) 2016 Rutgers University and Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar
//        and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include "mpi.h"
#include "fenix-config.h"
#include "fenix_ext.h"
#include "fenix_data_subset.h"


int __fenix_data_subset_init(int num_blocks, Fenix_Data_subset* subset){
   int retval = -1;
   if(num_blocks <= 0){
      debug_print("ERROR __fenix_data_subset_init: num_regions <%d> must be positive\n",
                num_blocks);
   } else {
      subset->start_offsets = (int*) s_malloc(sizeof(int) * num_blocks);
      subset->end_offsets = (int*) s_malloc(sizeof(int) * num_blocks);
      subset->num_repeats = (int*) s_calloc(num_blocks, sizeof(int));
      subset->num_blocks = num_blocks;
      retval = FENIX_SUCCESS;
   }
   return retval;
}

/**
 * @brief
 * @param num_blocks
 * @param start_offset
 * @param end_offset
 * @param stride
 * @param subset_specifier
 *
 * This routine creates 
 */
int __fenix_data_subset_create(int num_blocks, int start_offset, int end_offset, int stride,
                       Fenix_Data_subset *subset_specifier) {
  int retval = -1;
  if (num_blocks <= 0) {
    debug_print("ERROR Fenix_Data_subset_create: num_blocks <%d> must be positive\n",
                num_blocks);
    retval = FENIX_ERROR_SUBSET_NUM_BLOCKS;
  } else if (start_offset < 0) {
    debug_print("ERROR Fenix_Data_subset_create: start_offset <%d> must be positive\n",
                start_offset);
    retval = FENIX_ERROR_SUBSET_START_OFFSET;
  } else if (end_offset < 0) {
    debug_print("ERROR Fenix_Data_subset_create: end_offset <%d> must be positive\n",
                end_offset);
    retval = FENIX_ERROR_SUBSET_END_OFFSET;
  } else if (stride <= 0) {
    debug_print("ERROR Fenix_Data_subset_create: stride <%d> must be positive\n", stride);
    retval = FENIX_ERROR_SUBSET_STRIDE;
  } else {
    //This is a simple subset with a single region descriptor that simply
    //repeats num_blocks times.
    __fenix_data_subset_init(1 /*Only 1 block, repeated*/, subset_specifier);

    subset_specifier->start_offsets[0] = start_offset;
    subset_specifier->end_offsets[0] = end_offset;
    subset_specifier->num_repeats[0] = num_blocks-1;
    subset_specifier->stride = stride;
    subset_specifier->specifier = __FENIX_SUBSET_CREATE;
    retval = FENIX_SUCCESS;
  }
  return retval;
}

/**
 * @brief
 * @param num_blocks
 * @param array_start_offsets
 * @param array_end_offsets
 * @param subset_specifier
 */
int __fenix_data_subset_createv(int num_blocks, int *array_start_offsets, int *array_end_offsets,
                        Fenix_Data_subset *subset_specifier) {

  int retval = -1;
  if (num_blocks <= 0) {
    debug_print("ERROR Fenix_Data_subset_createv: num_blocks <%d> must be positive\n",
                num_blocks);
    retval = FENIX_ERROR_SUBSET_NUM_BLOCKS;
  } else if (array_start_offsets == NULL) {
    debug_print( "ERROR Fenix_Data_subset_createv: array_start_offsets %s must be at least of size 1\n", "");
    retval = FENIX_ERROR_SUBSET_START_OFFSET;
  } else if (array_end_offsets == NULL) {
    debug_print( "ERROR Fenix_Data_subset_createv: array_end_offsets %s must at least of size 1\n", "");
    retval = FENIX_ERROR_SUBSET_END_OFFSET;
  } else {

    // first check that the start offsets and end offsets are valid
    int index;
    int invalid_index = -1;
    int found_invalid_index = 0;
    for (index = 0; found_invalid_index != 1 && (index < num_blocks); index++) {
      if (array_start_offsets[index] > array_end_offsets[index]) {
        invalid_index = index;
        found_invalid_index = 1;
      }
    }

    if (found_invalid_index != 1) { // if not true (!= 1)
      __fenix_data_subset_init(num_blocks, subset_specifier);

      memcpy(subset_specifier->start_offsets, array_start_offsets, ( num_blocks * sizeof(int))); // deep copy
      memcpy(subset_specifier->end_offsets, array_end_offsets, ( num_blocks * sizeof(int))); // deep copy
      
      subset_specifier->specifier = __FENIX_SUBSET_CREATEV;
      retval = FENIX_SUCCESS;
    } else {
      debug_print(
              "ERROR Fenix_Data_subset_createv: array_end_offsets[%d] must be less than array_start_offsets[%d]\n",
              invalid_index, invalid_index);
      retval = FENIX_ERROR_SUBSET_END_OFFSET;
    }
  }
  return retval;
}

//This should only be used to copy to a currently non-inited subset
// If the destination already has memory allocated in the num_blocks/offsets regions
// then this can lead to memory leaks.
void __fenix_data_subset_deep_copy(Fenix_Data_subset* from, Fenix_Data_subset* to){
   if(from->specifier == __FENIX_SUBSET_FULL || from->specifier == __FENIX_SUBSET_EMPTY){
      to->specifier = from->specifier;
   } else {
      __fenix_data_subset_init(from->num_blocks, to);
      memcpy(to->num_repeats, from->num_repeats, to->num_blocks*sizeof(int));
      memcpy(to->start_offsets, from->start_offsets, to->num_blocks*sizeof(int));
      memcpy(to->end_offsets, from->end_offsets, to->num_blocks*sizeof(int));
      to->specifier = from->specifier;
      to->stride = from->stride;
   }
}

//This function checks for any overlapping regions and removes them.
void __fenix_data_subset_simplify_regions(Fenix_Data_subset* ss){
   int space_allocated = ss->num_blocks;
  
   if(ss->specifier == __FENIX_SUBSET_CREATE){
      //We will handle this by viewing the data as regions of size stride.
      //Each block will be broken into a value dictating which regions it is
      //within, and what data within each region it is within.
      //
      //If two blocks do not overlap within regions, there is no overlap.
      //If they overlap within regions, but the regions they touch do not overlap,
      //there is no overlap. etc.
      
      for(int i = 0; i < ss->num_blocks-1; i++){
         int did_merge = 0;
         
         for(int j = i+1; j < ss->num_blocks; j++){ 
            //We will simplify the logic by switching from i and j referencing
            //to viewing the two blocks in the order that they exist in the data.
            int first_block; 
            int second_block;

            if(ss->start_offsets[i] < ss->start_offsets[j]){
               first_block = i;
               second_block = j;
            } else {
               first_block = j;
               second_block = i;
            }

            //Check for the case that the merged and unmerged regions are the same.
            int merged_same_as_unmerged = ((ss->start_offsets[first_block]%ss->stride) == (ss->start_offsets[second_block]%ss->stride)
                  && (ss->end_offsets[first_block]%ss->stride) == (ss->end_offsets[second_block]%ss->stride));
            int merged_same_as_first = 0, merged_same_as_second = 0;

            
            // We want the smallest x | (first_block_end + stride * x >= second_block_start)
            // As this gives us which repetition an overlap is first possible on.
            // Simplify to x >= (second_block_start - first_block_end)/s
            // We want the lowest, so swap >= with =, and since we need an integer we'll round up.
            int first_intersecting_repetition, option2;
            if(ss->start_offsets[second_block] - ss->end_offsets[first_block] > 0){
               first_intersecting_repetition = (ss->start_offsets[second_block] - ss->end_offsets[first_block] - 1)/ss->stride + 1;
               // = ceil( (ss->start_offsets[second_block] - ss->end_offsets[first_block]) / ss->stride)
            } else {
               first_intersecting_repetition = 0;
            }
            
            // The above only accounts for one of two cases of intersection. There other is provided by option 2.
            if(ss->end_offsets[second_block] - ss->end_offsets[first_block] > 0){
               option2 = (ss->end_offsets[second_block] - ss->end_offsets[first_block] - 1)/ss->stride + 1;
            } else {
               option2 = 0;
            }

            if(merged_same_as_unmerged){
               //If there's no difference in merged/unmerged, we can 'skip' a stride 
               //and it'll al still be the same.
               if(!( first_intersecting_repetition <= ss->num_repeats[first_block]+1 
                        || option2 <= ss->num_repeats[first_block]+1 )){
                  //Both still require too high a repetition than we have. No overlap.
                  continue;
               }
            } else {
               if(!( first_intersecting_repetition <= ss->num_repeats[first_block] 
                        || option2 <= ss->num_repeats[first_block] )){
                  //Both require too high a repetition than we have. No overlap.
                  continue;
               }
            }

            merged_same_as_first = ss->start_offsets[first_block] + ss->stride*first_intersecting_repetition <= ss->start_offsets[second_block]
                  && ss->end_offsets[first_block] + ss->stride*first_intersecting_repetition >= ss->end_offsets[second_block];
            merged_same_as_second = ss->start_offsets[second_block] + ss->stride*first_intersecting_repetition <= ss->start_offsets[first_block]
                  && ss->end_offsets[second_block] + ss->stride*first_intersecting_repetition >= ss->end_offsets[first_block];
            
            //We have found the smallest overlap candidate, now we see if there is overlap there.
            int blocks_overlap;
            if(first_intersecting_repetition < option2){
               blocks_overlap = ( (ss->stride * first_intersecting_repetition + ss->start_offsets[first_block]) <= ss->start_offsets[second_block]);
            } else {
               first_intersecting_repetition = option2;
               blocks_overlap = ( (ss->stride * first_intersecting_repetition + ss->start_offsets[first_block]) <= ss->end_offsets[second_block]);
            }

            if(!blocks_overlap){
               continue;
            }

            int length_first_only_start;
            int length_first_only_end;
            int length_both;
            int length_second_only;
            int merged_start;
            int merged_end;
            
            
            length_first_only_start = first_intersecting_repetition;
            
            length_first_only_start = length_first_only_start > (ss->num_repeats[i] + 1) ?
                  (ss->num_repeats[i] + 1) : length_first_only_start;

            int remaining_first_repetitions = ss->num_repeats[first_block] + 1 - length_first_only_start;
            if(remaining_first_repetitions > ss->num_repeats[second_block]+1){
               length_both = ss->num_repeats[second_block] + 1;
               length_second_only = 0;
               length_first_only_end = ss->num_repeats[first_block]+1 - length_first_only_start - length_both;
            } else {
               length_both = remaining_first_repetitions;
               length_second_only = ss->num_repeats[second_block]+1 - remaining_first_repetitions;
               length_first_only_end = 0;
            } 
            
            if(merged_same_as_unmerged){
               length_both = length_both + length_first_only_end + length_first_only_start + length_second_only;
               length_first_only_start = length_first_only_end = length_second_only = 0;
            } else if(merged_same_as_first){
               length_both = length_both + length_first_only_end + length_first_only_start;

               length_first_only_start = length_first_only_end = 0;
            } else if(merged_same_as_second){
               length_both = length_both + length_both;

               length_both = 0;
            }
            
            //Record info for merged region before we overwrite data we need.
            merged_start = ss->stride*length_first_only_start + ss->start_offsets[first_block];
            merged_start = merged_start < ss->start_offsets[second_block] ?
                  merged_start : ss->start_offsets[second_block];
            
            merged_end = ss->stride*length_first_only_start + ss->end_offsets[first_block];
            merged_end = merged_end < ss->end_offsets[second_block] ?
                  merged_end : ss->end_offsets[second_block];            

            //Now we know what the overlap is, so we make the changes to the data subset.
            int store_index = 0;
            int store_locations[3] = {first_block, second_block, ss->num_blocks};

            if(length_first_only_start > 0){
               ss->num_repeats[first_block] = length_first_only_start - 1;
               store_index++;
            }
            if(length_first_only_end > 0){
               ss->num_repeats[store_locations[store_index]] = length_first_only_end-1;
               ss->start_offsets[store_locations[store_index]] = ss->stride*(length_first_only_start+length_both) + 
                     ss->start_offsets[first_block];
               ss->end_offsets[store_locations[store_index]] = ss->stride*(length_first_only_start+length_both) + 
                     ss->end_offsets[first_block];
               store_index++;
            } else if(length_second_only > 0){
               ss->num_repeats[store_locations[store_index]] = length_second_only-1;
               ss->start_offsets[store_locations[store_index]] = ss->stride*(length_both) + 
                     ss->start_offsets[second_block];
               ss->end_offsets[store_locations[store_index]] = ss->stride*(length_both) + 
                     ss->end_offsets[second_block];
               store_index++; 
            }

            //There is always a merged region to add.
            if(store_index == 2){
               //We're adding a new block, so we need to make sure we have allocated
               //enough memory space.
               ss->num_blocks++;
               if(ss->num_blocks > space_allocated){
                  
                  ss->end_offsets = (int*) s_realloc(ss->end_offsets,
                                (space_allocated * 2) * sizeof(int));
                  ss->start_offsets = (int*) s_realloc(ss->start_offsets,
                                (space_allocated * 2) * sizeof(int));
                  ss->num_repeats = (int*) s_realloc(ss->num_repeats,
                                (space_allocated * 2) * sizeof(int));
                  space_allocated *= 2;
               }

            }
            ss->start_offsets[store_locations[store_index]] = merged_start;
            ss->end_offsets[store_locations[store_index]] = merged_end;
            ss->num_repeats[store_locations[store_index]] = length_both - 1;
            store_index++;
 

            //Check if num_repeats[second_block] < 0, if so remove it.
            //This could occur if both blocks can be perfectly minimized to a single block.
            if(store_index == 1){
               if(second_block == ss->num_blocks-1){
                  //Don't need to move anything.
                  ss->num_blocks--;
               } else {
                  //We need to move everything over by one.
                  memmove(ss->num_repeats + second_block, ss->num_repeats + second_block + 1, 
                        ss->num_blocks - second_block - 1);
                  memmove(ss->start_offsets + second_block, ss->start_offsets + second_block + 1, 
                        ss->num_blocks - second_block - 1);
                  memmove(ss->end_offsets + second_block, ss->end_offsets + second_block + 1, 
                        ss->num_blocks - second_block - 1);
                  ss->num_blocks--;
               }
            } 
            
            did_merge = 1;
         }

         //If we merged w/ anything, recheck w/ new merged block.
         if(did_merge) i--;
      }
   } else if(ss->specifier == __FENIX_SUBSET_CREATEV){
      //This is much simpler than with CREATE type, since we don't have to
      //worry about repetition.
      for(int i = 0; i < ss->num_blocks-1; i++){
         int did_merge = 0;
         
         for(int j = i+1; j < ss->num_blocks; j++){
            if(   ss->start_offsets[i] <= ss->end_offsets[j]+1 &&
                  ss->end_offsets[i] >= ss->start_offsets[j]-1){
               did_merge = 1;

               ss->start_offsets[i] = (ss->start_offsets[i] < ss->start_offsets[j]) ?
                     ss->start_offsets[i] :
                     ss->start_offsets[j];

               ss->end_offsets[i] = (ss->end_offsets[i] > ss->end_offsets[j]) ?
                     ss->end_offsets[i] :
                     ss->end_offsets[j];
               
               //Move everything over to remove j
               memmove(ss->start_offsets + j, ss->start_offsets + j + 1, 
                     (ss->num_blocks - j - 1) * sizeof(int));
               memmove(ss->end_offsets + j, ss->end_offsets + j + 1, 
                     (ss->num_blocks - j - 1) * sizeof(int));
               ss->num_blocks--;
            }
         }

         if(did_merge) i--;
      }
   }

   if(space_allocated > ss->num_blocks){
      ss->end_offsets = (int*) s_realloc(ss->end_offsets,
                    ss->num_blocks * sizeof(int));
      ss->start_offsets = (int*) s_realloc(ss->start_offsets,
                    ss->num_blocks * sizeof(int));
      ss->num_repeats = (int*) s_realloc(ss->num_repeats,
                    ss->num_blocks * sizeof(int));
   }

}

//This should only be used to copy to a currently non-inited subset
// If the destination already has memory allocated in the num_blocks/offsets regions
// then this can lead to double-mallocs or memory leaks.
void __fenix_data_subset_merge(Fenix_Data_subset* first_subset, Fenix_Data_subset* second_subset,
      Fenix_Data_subset* output){
      
   //Simple cases first
   if(first_subset->specifier == __FENIX_SUBSET_FULL || 
         second_subset->specifier == __FENIX_SUBSET_FULL){
      //We don't need to populate anything else.
      output->specifier = __FENIX_SUBSET_FULL;
      //We still have to init, else there will be a memory error when the user tries to free later.
      __fenix_data_subset_init(1, output);
   
   } else if(first_subset->specifier == __FENIX_SUBSET_EMPTY){
      __fenix_data_subset_deep_copy(second_subset, output);
   
   } else if(second_subset->specifier == __FENIX_SUBSET_EMPTY){
      __fenix_data_subset_deep_copy(first_subset, output);

   } else if(first_subset->specifier == __FENIX_SUBSET_CREATE &&
         second_subset->specifier == __FENIX_SUBSET_CREATE &&
         first_subset->stride == second_subset->stride){
      //Output is just a CREATE type with combined descriptors. 
      //Start by making a list of all descriptors, then merge any with overlaps.
      output->stride = first_subset->stride;
      output->num_blocks = first_subset->num_blocks 
         + second_subset->num_blocks;
      __fenix_data_subset_init(output->num_blocks, output);
      output->specifier = __FENIX_SUBSET_CREATE;
      
      memcpy(output->num_repeats, first_subset->num_repeats, first_subset->num_blocks * sizeof(int));
      memcpy(output->num_repeats+first_subset->num_blocks, second_subset->num_repeats, 
            second_subset->num_blocks * sizeof(int));

      memcpy(output->start_offsets, first_subset->start_offsets, first_subset->num_blocks * sizeof(int));
      memcpy(output->start_offsets+first_subset->num_blocks, second_subset->start_offsets, 
            second_subset->num_blocks * sizeof(int));
      
      memcpy(output->end_offsets, first_subset->end_offsets, first_subset->num_blocks * sizeof(int));
      memcpy(output->end_offsets+first_subset->num_blocks, second_subset->end_offsets, 
            second_subset->num_blocks * sizeof(int));
   
      //Now we have all of the regions, so we just need to simplify them.
      __fenix_data_subset_simplify_regions(output); 
   } else {
      output->specifier = __FENIX_SUBSET_CREATEV;
      
      output->num_blocks = first_subset->num_blocks + second_subset->num_blocks;
      if(first_subset->specifier == __FENIX_SUBSET_CREATE){
         for(int i = 0; i < first_subset->num_blocks; i++){
            output->num_blocks += first_subset->num_repeats[i];
         }
      }
      if(second_subset->specifier == __FENIX_SUBSET_CREATE){
         for(int i = 0; i < second_subset->num_blocks; i++){
            output->num_blocks += second_subset->num_repeats[i];
         }
      }

      __fenix_data_subset_init(output->num_blocks, output);

      int index = 0;
      for(int i = 0; i < first_subset->num_blocks; i++){
         for(int j = 0; j <= first_subset->num_repeats[i]; j++){
            output->start_offsets[index] = j*first_subset->stride + first_subset->start_offsets[i];
            output->end_offsets[index] = j*first_subset->stride + first_subset->end_offsets[i];
            index++;
         }
      }
      for(int i = 0; i < second_subset->num_blocks; i++){
         for(int j = 0; j <= second_subset->num_repeats[i]; j++){
            output->start_offsets[index] = j*second_subset->stride + second_subset->start_offsets[i];
            output->end_offsets[index] = j*second_subset->stride + second_subset->end_offsets[i];
            index++;
         }
      }

      //Now we have all of the regions, so we just need to simplify them.
      __fenix_data_subset_simplify_regions(output);
   }
}

//Merge second subset into first subset
//This reasonably assumes both subsets are already intialized.
void __fenix_data_subset_merge_inplace(Fenix_Data_subset* first_subset, Fenix_Data_subset* second_subset){
      
   //Simple cases first
   if(first_subset->specifier == __FENIX_SUBSET_FULL || 
         second_subset->specifier == __FENIX_SUBSET_FULL){
      //We don't need to populate anything else.
      first_subset->specifier = __FENIX_SUBSET_FULL;

   } else if(second_subset->specifier == __FENIX_SUBSET_EMPTY){
      //Do nothing.
      
   } else if(first_subset->specifier  == __FENIX_SUBSET_EMPTY){
      //Deep copy requires that the destination be non-initialized, so free sub1 first.
      __fenix_data_subset_free(first_subset);
      __fenix_data_subset_deep_copy(second_subset, first_subset);

   } else if(first_subset->specifier == __FENIX_SUBSET_CREATE &&
         second_subset->specifier == __FENIX_SUBSET_CREATE &&
         first_subset->stride == second_subset->stride){
      //Output is just a CREATE type with combined descriptors. 
      //Start by making a list of all descriptors, then merge any with overlaps.
      first_subset->num_repeats = (int*)s_realloc(first_subset->num_repeats, 
            (first_subset->num_blocks + second_subset->num_blocks)*sizeof(int));
      first_subset->start_offsets = (int*)s_realloc(first_subset->start_offsets, 
            (first_subset->num_blocks + second_subset->num_blocks)*sizeof(int));
      first_subset->end_offsets = (int*)s_realloc(first_subset->end_offsets, 
            (first_subset->num_blocks + second_subset->num_blocks)*sizeof(int));

      memcpy(first_subset->num_repeats+first_subset->num_blocks, second_subset->num_repeats, 
            second_subset->num_blocks * sizeof(int));

      memcpy(first_subset->start_offsets+first_subset->num_blocks, second_subset->start_offsets, 
            second_subset->num_blocks * sizeof(int));
      
      memcpy(first_subset->end_offsets+first_subset->num_blocks, second_subset->end_offsets, 
            second_subset->num_blocks * sizeof(int));
      
      first_subset->num_blocks = first_subset->num_blocks 
         + second_subset->num_blocks;
   
      //Now we have all of the regions, so we just need to simplify them.
      __fenix_data_subset_simplify_regions(first_subset); 
   } else {
      
      int new_num_blocks = first_subset->num_blocks + second_subset->num_blocks;
      if(first_subset->specifier == __FENIX_SUBSET_CREATE){
         for(int i = 0; i < first_subset->num_blocks; i++){
            new_num_blocks += first_subset->num_repeats[i];
         }
      }
      if(second_subset->specifier == __FENIX_SUBSET_CREATE){
         for(int i = 0; i < second_subset->num_blocks; i++){
            new_num_blocks += second_subset->num_repeats[i];
         }
      }

      first_subset->num_repeats = (int*)s_realloc(first_subset->num_repeats, new_num_blocks*sizeof(int));
      first_subset->start_offsets = (int*)s_realloc(first_subset->start_offsets, new_num_blocks*sizeof(int));
      first_subset->end_offsets = (int*)s_realloc(first_subset->end_offsets, new_num_blocks*sizeof(int));

      //work backwards to prevent overwriting current data.

      int index = new_num_blocks-1;
      for(int i = second_subset->num_blocks-1; i >= 0; i--){
         for(int j = 0; j <= second_subset->num_repeats[i]; j++){
            first_subset->start_offsets[index] = j*second_subset->stride 
               + second_subset->start_offsets[i];
            first_subset->end_offsets[index] = j*second_subset->stride 
               + second_subset->end_offsets[i];
            first_subset->num_repeats[index] = 0;
            index--;
         }
      }
      for(int i = first_subset->num_blocks-1; i >= 0; i--){
         for(int j = 0; j <= first_subset->num_repeats[i]; j++){
            first_subset->start_offsets[index] = j*first_subset->stride 
               + first_subset->start_offsets[i];
            first_subset->end_offsets[index] = j*first_subset->stride 
               + first_subset->end_offsets[i];
            first_subset->num_repeats[index] = 0;
            index--;
         }
      }
      first_subset->specifier = __FENIX_SUBSET_CREATEV;
      first_subset->num_blocks = new_num_blocks;

      //Now we have all of the regions, so we just need to simplify them.
      __fenix_data_subset_simplify_regions(first_subset);
   }

}


void __fenix_data_subset_copy_data(Fenix_Data_subset* ss, void* dest, void* src, size_t data_type_size){
   for(int i = 0; i < ss->num_blocks; i++){
      //Inclusive both directions, so add 1.
      int length = ss->end_offsets[i]-ss->start_offsets[i] + 1;
      
      for(int j = 0; j <= ss->num_repeats[i]; j++){
         int start = ss->start_offsets[i] + j*ss->stride;
         printf("Copying from %d to %d, block %d repeat %d\n", start, start+length-1, i, j);
         memcpy( ((uint8_t*)dest) + start*data_type_size, ((uint8_t*)src) + start*data_type_size, length*data_type_size);
      }
   }
}

int __fenix_data_subset_data_size(Fenix_Data_subset* ss, size_t max_size){
   int size;

   if(ss->specifier == __FENIX_SUBSET_FULL){
      size = max_size;
   } else if( ss->specifier == __FENIX_SUBSET_EMPTY){
      size = 0;
   } else {
      size = 0;
      for(int i = 0; i < ss->num_blocks; i++){
         size += (ss->end_offsets[i] - ss->start_offsets[i] + 1)*(ss->num_repeats[i]+1);
      }
   }

   return size;
}

int __fenix_data_subset_is_full(Fenix_Data_subset *ss, size_t data_length){
   return (ss->specifier == __FENIX_SUBSET_FULL) || 
      ( (ss->start_offsets[0] == 0) && (ss->end_offsets[0] == data_length-1) );
}

//Makes an array with the in-order contents of subset ss of src.
//size is updated to the size of the serialized array, which is returned as the function's return.
//User's responsibility to free the returned array.
void* __fenix_data_subset_serialize(Fenix_Data_subset* ss, void* src, size_t type_size, size_t max_size, size_t* size){
   
   void* dest;
   
   if(ss->specifier == __FENIX_SUBSET_FULL){
      dest = malloc(type_size*max_size);

      memcpy(dest, src, type_size*max_size);

      *size = max_size;

   } else if(ss->specifier == __FENIX_SUBSET_EMPTY) {

      dest = NULL;
      size = 0;

   } else {
      //First, count up the number of entries to find a size.
      *size = __fenix_data_subset_data_size(ss, max_size);

      dest = malloc(type_size * (*size));
      
      int* current_repetition = (int*) s_calloc(ss->num_blocks, sizeof(int));
      //We need to be sure to go in the right order.
      int stored = 0;
      while(stored < *size){
         int lowest_index = -1;
         int lowest_block = -1;
         for(int i = 0; i < ss->num_blocks; i++){
            if(current_repetition[i] <= ss->num_repeats[i]){
               if(lowest_index == -1 || 
                     (lowest_index > ss->start_offsets[i]+ss->stride*current_repetition[i])){
                  lowest_index = ss->start_offsets[i] + ss->stride*current_repetition[i];
                  lowest_block = i;
               }
            }
         }
         
         memcpy(((uint8_t*)dest)+stored*type_size, ((uint8_t*)src)+lowest_index*type_size, 
               type_size*(ss->end_offsets[lowest_block]-ss->start_offsets[lowest_block]+1) );
         stored += ss->end_offsets[lowest_block]-ss->start_offsets[lowest_block]+1;
         current_repetition[lowest_block]++;
      }

      free(current_repetition);

   }


   return dest;
}

void __fenix_data_subset_deserialize(Fenix_Data_subset* ss, void* src, void* dest, size_t max_size, size_t type_size){
   if(ss->specifier == __FENIX_SUBSET_FULL){
      memcpy(dest, src, type_size*max_size);
      
   } else if(ss->specifier != __FENIX_SUBSET_EMPTY){
      //First, count up the number of entries to find a size.
      int size = __fenix_data_subset_data_size(ss, max_size);
    
      int* current_repetition = (int*) s_calloc(ss->num_blocks, sizeof(int));
      //We need to be sure to go in the right order.
      int restored = 0;
      while(restored < size){
         int lowest_index = -1;
         int lowest_block = -1;
         for(int i = 0; i < ss->num_blocks; i++){
            if(current_repetition[i] <= ss->num_repeats[i]){
               if(lowest_index == -1 || 
                     (lowest_index > ss->start_offsets[i]+ss->stride*current_repetition[i])){
                  lowest_index = ss->start_offsets[i] + ss->stride*current_repetition[i];
                  lowest_block = i;
               }
            }
         }
         
         memcpy(((uint8_t*)dest)+lowest_index*type_size, ((uint8_t*)src)+restored*type_size, 
               type_size*(ss->end_offsets[lowest_block]-ss->start_offsets[lowest_block]+1) );
         restored += ss->end_offsets[lowest_block]-ss->start_offsets[lowest_block]+1;
         current_repetition[lowest_block]++;
      }

      free(current_repetition);
   }

}

void __fenix_data_subset_send(Fenix_Data_subset* ss, int dest, int tag, MPI_Comm comm){
   int* toSend = (int*)malloc(sizeof(int) * (3 + 3*ss->num_blocks));
   toSend[0] = ss->num_blocks;
   
   for(int i = 0; i < ss->num_blocks; i++){
      toSend[1+3*i] = ss->start_offsets[i];
      toSend[2+3*i] = ss->end_offsets[i];
      toSend[3+3*i] = ss->num_repeats[i];
   }

   toSend[1+3*ss->num_blocks] = ss->stride;
   toSend[2+3*ss->num_blocks] = ss->specifier;

   MPI_Send((void*)toSend, 3*ss->num_blocks + 3, MPI_INT, dest, tag, comm); 
   free(toSend);
}

void __fenix_data_subset_recv(Fenix_Data_subset* ss, int src, int tag, MPI_Comm comm){
   MPI_Status status;
   MPI_Probe(src, tag, comm, &status);

   int size;
   MPI_Get_count(&status, MPI_INT, &size);

   int *recvd = (int*)malloc(sizeof(int) * size);
   MPI_Recv((void*)recvd, size, MPI_INT, src, tag, comm, NULL);

   __fenix_data_subset_init(recvd[0], ss);
   for(int i = 0; i < ss->num_blocks; i++){
      ss->start_offsets[i] = recvd[1+3*i];
      ss->end_offsets[i] = recvd[2+3*i];
      ss->num_repeats[i] = recvd[3+3*i];
   }
   ss->stride = recvd[1+3*ss->num_blocks];
   ss->specifier = recvd[2+3*ss->num_blocks];

   free(recvd);
}


int __fenix_data_subset_free( Fenix_Data_subset *subset_specifier ) {
  int  retval = FENIX_SUCCESS;
  free( subset_specifier->num_repeats );
  free( subset_specifier->start_offsets );
  free( subset_specifier->end_offsets );
  subset_specifier->specifier = __FENIX_SUBSET_UNDEFINED;
  return retval;
}

/**
 * @brief
 * @param subset_specifier
 */
int __fenix_data_subset_delete( Fenix_Data_subset *subset_specifier ) {
  __fenix_data_subset_free(subset_specifier);
  free(subset_specifier);
  return FENIX_SUCCESS;
}
