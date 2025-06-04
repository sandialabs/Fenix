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
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar,
//        Michael Heroux, and Matthew Whitlock
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include "mpi.h"
#include "fenix.h"
#include "fenix_opt.hpp"
#include "fenix_util.hpp"
#include "fenix_data_subset.h"
#include "fenix_data_subset.hpp"

namespace Fenix {
namespace Detail {

std::pair<size_t, size_t> DataRegion::range() const {
   return {start, reps == MAX ? MAX : end+stride*reps};
}

size_t DataRegion::count() const {
   if(end == MAX || reps == MAX) return MAX;
   return (end-start+1)*(reps+1);
}

bool DataRegion::operator==(const DataRegion& other) const {
   return start == other.start && end == other.end && reps == other.reps &&
      stride == other.stride;
}

bool DataRegion::operator&&(const DataRegion& b) const {
   const auto& a = *this;

   auto [astart, aend] = a.range();
   auto [bstart, bend] = b.range();
   if(astart > bend || bstart > aend) return false;
   else if(!a.reps || !b.reps) return true;

   //Both are strided. For now, only allow same-stride operations
   fenix_assert(a.stride == b.stride);

   //Get a big modularly idempotent number to avoid negatives
   size_t idem = (std::max(aend, bend)/stride + 1)*stride;

   //Start of possible overlap
   size_t ostart = std::max(astart, bstart);
   size_t arep = (ostart-astart)/stride;
   size_t brep = (ostart-bstart)/stride;
   DataRegion ablock = a.get_rep(arep);
   DataRegion bblock = b.get_rep(brep);
   
   if(ablock && bblock) return true;
   if(arep < a.reps && (bblock && a.get_rep(arep+1))) return true;
   if(brep < b.reps && (ablock && b.get_rep(brep+1))) return true;
   return false;
}

bool DataRegion::operator<(const DataRegion& other) const {
   if(start != other.start) return start < other.start;
   if(end != other.end) return end < other.end;
   if(reps != other.reps) return reps < other.reps;
   return stride < other.stride;
}

std::set<DataRegion> DataRegion::operator&(const DataRegion& b) const {
   fenix_assert(!reps || !b.reps);
   if(b.reps) return b & *this;

   if(!(*this && b)){
      return {};
   } else if(!reps){
      return {DataRegion({std::max(start, b.start), std::min(end, b.end)})};
   }

   std::set<DataRegion> ret;
   
   size_t start_rep = 0;
   if(b.start > start){
      start_rep = (b.start-start)/stride;
      auto block = get_rep(start_rep);
      auto valid = block & b;
      if(!valid.count(block)){
         ret.merge(valid);
         if(start_rep == reps) return ret;
         else start_rep++;
      }
   }

   size_t end_rep = reps;
   if(b.end < range().second){
      end_rep = (b.end-start)/stride;
      if(end_rep < start_rep) return ret;

      auto block = get_rep(end_rep);
      auto valid = block & b;
      if(!valid.count(block)){
         ret.merge(valid);
         if(end_rep == start_rep) return ret;
         else end_rep--;
      }
   }

   ret.insert(get_reps(start_rep, end_rep));
   return ret;
}

DataRegion DataRegion::get_rep(size_t n) const {
   fenix_assert(n <= reps);
   return DataRegion({start+n*stride, end+n*stride});
}

DataRegion DataRegion::get_reps(size_t first, size_t last) const {
   fenix_assert(first <= last);
   fenix_assert(last <= reps);
   return DataRegion(
      {start+first*stride, end+first*stride}, last-first, stride
   );
}

DataRegion DataRegion::inverted() const {
   fenix_assert(reps);
   return DataRegion({end+1, start+stride-1}, reps == MAX ? MAX : reps-1, stride);
}

std::optional<DataRegion> DataRegion::try_merge(const DataRegion& b) const {
   if(b < *this){
      return b.try_merge(*this);
   } else if(range().second == MAX){
      return {};
   } else if(!reps && !b.reps){
      if(end+1 == b.start) return DataRegion({start, b.end});
      else return {};
   } else {
      size_t s = reps ? stride : b.stride;
      if(start+s*(reps+1) == b.start && end+s*(reps+1) == b.end)
         return DataRegion({start, end}, reps+b.reps+1, s);
      else return {};
   }
}

void merge_adjacent_sets(std::set<DataRegion>& a, std::set<DataRegion>& b){
   if(a.empty()){
      a = std::move(b);
      b = {};
   }
   while(!b.empty()){
      auto merged = (--a.end())->try_merge(*b.begin());
      if(!merged){
         a.merge(b);
         b.clear();
      } else {
         a.erase(a.end()--);
         b.erase(b.begin());
         a.insert(*merged);
      }
   }
   while(a.size() > 1){
      auto merged = (----a.end())->try_merge(*(--a.end()));
      if(!merged) break;
      a.erase(a.end()--);
      a.erase(a.end()--);
      a.insert(*merged);
   }
}

std::set<DataRegion> DataRegion::operator-(const DataRegion& b) const {
   if(!(*this && b)) return {*this};
   fenix_assert(!reps || !b.reps || stride == b.stride);

   const auto [bstart, bend] = b.range();

   std::set<DataRegion> ret;
   if(bstart > 0) ret.merge(*this & DataRegion({0, bstart-1}));
   if(b.reps){
      std::set<DataRegion> mid;
      auto inv = b.inverted();
      
      //Get just the portions of myself that could be overlapping
      auto parts = *this & DataRegion({bstart, bend});

      //Handle any non-strided portions individually
      for(auto it = parts.begin(); it != parts.end();){
         if(it->reps){
            it++;
            continue;
         } else {
            mid.merge(*it & inv);
            it = parts.erase(it);
         }
      }

      //Should have taken care of everything, or left only 1 strided region
      if(!parts.empty()){
         fenix_assert(parts.size() == 1);
         auto a = *parts.begin();
         //First, middle, and last repetitions may be effected differently
         mid.merge(a.get_rep(0) & inv);
         if(a.reps > 1){
            size_t middle_reps = a.reps-2;
            auto middle_valid = a.get_rep(1) & inv;
            for(auto block : middle_valid){
               fenix_assert(!block.reps);
               mid.insert(DataRegion(
                  {block.start, block.end}, a.reps-2, b.stride
               ));
            }
         }
         mid.merge(a.get_rep(a.reps) & inv);
      }
      merge_adjacent_sets(ret, mid);
   }
   if(bend != MAX){
      auto post = *this & DataRegion({bend+1, MAX});
      merge_adjacent_sets(ret, post);
   }
   return ret;
}

std::string DataRegion::str() const {
   std::string ret = "[";

   if(start == MAX) ret += "MAX";
   else ret += std::to_string(start);

   ret += ",";

   if(end == MAX) ret += "MAX";
   else ret += std::to_string(end);

   ret += "]";

   if(reps){
      ret += "x" + std::to_string(reps+1) + "s" + std::to_string(stride);
   }
   return ret;
}

DataRegion full_region({0, DataRegion::MAX});

struct BlockIter {
   using set_t = std::set<DataRegion>;
   using iter_t = typename set_t::iterator;

   BlockIter(set_t&& m_regions) 
      : region_holder(std::make_shared<set_t>(std::move(m_regions))),
        regions(*region_holder), it(regions.begin()), rep(0) { }
   BlockIter(std::shared_ptr<set_t> m_regions) 
      : region_holder(m_regions),
        regions(*region_holder), it(regions.begin()), rep(0) { }
   BlockIter(const BlockIter& other)
      : region_holder(other.region_holder), regions(other.regions), it(other.it), rep(other.rep) {
      fenix_assert(rep <= it->reps);
      ++*this;
   }

   DataRegion operator*(){
      return it->get_rep(rep);
   }

   bool operator==(const BlockIter& other) const {
      return it == other.it && rep == other.rep;
   }
   bool operator!=(const BlockIter& other) const {
      return !(*this == other);
   }

   BlockIter begin(){
      return BlockIter(region_holder);
   }

   BlockIter end(){
      auto ret = begin();
      ret.it = regions.end();
      return ret;
   }

   BlockIter& operator++(){
      if(rep == it->reps){
         ++it;
         rep = 0;
      } else {
         ++rep;
      }
      return *this;
   }
   
   std::shared_ptr<set_t> region_holder;

   const set_t& regions;
   iter_t it;
   size_t rep;

};

} // namespace Detail

using namespace Detail;

DataSubset::DataSubset(size_t end) : DataSubset({0, end}) { }

DataSubset::DataSubset(std::pair<size_t, size_t> bounds)
   : DataSubset(bounds, 1, DataRegion::MAX) { };

DataSubset::DataSubset(
   std::pair<size_t, size_t> bounds, size_t n, size_t stride
) {
   fenix_assert(bounds.first <= bounds.second,
      "subset start (%lu) cannot be after end (%lu)",
      bounds.first, bounds.second
   );
   fenix_assert(n > 0, "num_blocks (%lu) must be positive", n);
   fenix_assert(n == 1 || bounds.first+stride > bounds.second,
      "stride %lu too low for region [%lu, %lu]",
      stride, bounds.first, bounds.second
   );
   
   regions.emplace(bounds, n-1, stride);
}

DataSubset::DataSubset(std::vector<std::pair<size_t, size_t>> bounds){
   for(const auto& b : bounds){
      fenix_assert(b.first <= b.second,
         "subset start (%lu) cannot be after end (%lu)", b.first, b.second
      );
      regions.emplace(b);
   }

   //Simplify regions
   merge_regions();
}

DataSubset::DataSubset(const DataSubset& a, const DataSubset& b){
   if(a.empty() || b.empty()){
      *this = a.empty() ? b : a;
      return;
   }
   if(a.regions.count(full_region) || b.regions.count(full_region)){
      regions.insert(full_region);
      return;
   }

   size_t a_stride = 0, b_stride = 0;
   for(const auto& r : a.regions) if(r.reps) a_stride = r.stride;
   for(const auto& r : b.regions) if(r.reps) b_stride = r.stride;

   if(a_stride && b_stride && a_stride != b_stride){
      //De-stride A's regions
      for(const auto& r : a.regions){
         fenix_assert(r.reps != MAX);
         for(int i = 0; i <= r.reps; i++)
            regions.insert(r.get_rep(i));
      }
   } else {
      regions = a.regions;
   }
   
   for(const auto& br : b.regions){
      const auto [bstart, bend] = br.range();
      std::set<DataRegion> adding = {br};
      for(const auto& r : regions){
         const auto [rstart, rend] = r.range();
         if(rstart > bend) break;
         if(rend < bstart) continue;
         for(auto it = adding.begin(); it != adding.end();){
            auto valid = *it - r;
            if(valid.size() == 1 && valid.count(*it)){
               it++;
            } else {
               it = adding.erase(it);
               adding.merge(valid);
            }
         }
      }
      regions.merge(adding);
   }
   
   //Final attempt to simplify all regions
   merge_regions();
}

void DataSubset::merge_regions() {
   auto check = regions.begin();
   while(check != regions.end()){
      auto crange = check->range();
     
      bool erase = false;
      for(auto i = std::next(check); i != regions.end(); i++){
         if(crange.second < i->start-1) break;

         size_t merge_idx = -1;
         size_t merge_reps = -1;

         size_t matching_end = i->start-1;
         size_t matching_start = i->end+1;
         if((matching_end - check->end)%check->stride == 0){
            merge_idx = (matching_end-check->end)/check->stride;
            merge_reps = std::min(check->reps-merge_idx, i->reps);

            //Add the merged region
            regions.insert(DataRegion(
               {check->start+merge_idx*check->stride, i->end},
               merge_reps, check->stride
            ));
         } else if((matching_start - check->start)%check->stride == 0){
            merge_idx = (matching_start-check->start)/check->stride;
            merge_reps = std::min(check->reps-merge_idx, i->reps);

            //Add the merged region
            regions.insert(DataRegion(
               {i->start, check->end+merge_idx*check->stride},
               merge_reps, check->stride
            ));
         }

         if(merge_idx != -1){
            erase = true;

            //Add any blocks before the merge
            if(merge_idx > 0){
               regions.insert(DataRegion(
                  {check->start, check->end}, merge_idx-1, check->stride
               ));
            }
            //Add any blocks after the merge
            if(merge_idx+merge_reps < check->reps){
               size_t n_pre = merge_idx+merge_reps+1;
               size_t offset = n_pre*check->stride;
               regions.insert(DataRegion(
                  {check->start+offset, check->end+offset},
                  check->reps-n_pre, check->stride
               ));
            } else if(merge_reps < i->reps){
               size_t n_pre = merge_reps+1;
               size_t offset = n_pre*i->stride;
               regions.insert(DataRegion(
                  {i->start+offset, i->end+offset},
                  i->reps-n_pre, i->stride
               ));
            }
            
            regions.erase(i);
            break;
         }
      }

      auto it = check++;
      if(erase){
         regions.erase(it);
      }
   }
}

DataSubset::DataSubset(const DataBuffer& buf){
   fenix_assert(buf.size()%sizeof(DataRegion) == 0);
   
   size_t n_regions = buf.size()/sizeof(DataRegion);
   if(n_regions == 0) return;

   DataRegion* r = (DataRegion*) buf.data();
   for(int i = 0; i < n_regions; i++){
      regions.insert(*(r++));
   }
}

void DataSubset::serialize(DataBuffer& buf) const {
   buf.reset(regions.size()*sizeof(DataRegion));
   DataRegion* r = (DataRegion*) buf.data();
   for(const auto& region : regions){
      *(r++) = region;
   }
}

DataSubset DataSubset::operator+(const DataSubset& other) const {
   return DataSubset(*this, other);
}

DataSubset DataSubset::operator+(const Fenix_Data_subset& other) const {
   return *this + *(DataSubset*)other.impl;
}

DataSubset& DataSubset::operator+=(const DataSubset& other) {
   *this = *this + other;
   return *this;
}

DataSubset& DataSubset::operator+=(const Fenix_Data_subset& other) {
   return *this += *(DataSubset*)other.impl;
}

DataSubset DataSubset::operator-(const DataSubset& other) const {
   if(empty() || other.empty()) return *this;

   size_t a_stride = 0, b_stride = 0;
   for(const auto& r :       regions) if(r.reps) a_stride = r.stride;
   for(const auto& r : other.regions) if(r.reps) b_stride = r.stride;

   fenix_assert(!(
      a_stride && b_stride && a_stride != b_stride &&
      end() == MAX && other.end() == MAX
   ));

   std::set<DataRegion> remaining;
   if(a_stride && b_stride && a_stride != b_stride){
      size_t b_end = other.end();

      //Insert individual repetitions possibly overlapping regions
      for(const auto& b : BlockIter(bounded_regions(0, b_end))){
         remaining.insert(b);
      }
      if(b_end != MAX){
         //Leave non-overlapping regions strided
         auto br = bounded_regions(b_end+1, MAX);
         for(const auto& b : br){
            remaining.insert(b);
         }
      }
   } else {
      remaining = regions;
   }

   for(const auto& b_r : other.regions){
      auto next = remaining.begin();
      while(next != remaining.end()){
         auto it = next++;

         if(b_r.start > it->range().second) continue;
         if(it->start > b_r.range().second) break;

         auto r = *it;
         remaining.erase(it);
         remaining.merge(r - b_r);
      }
   }

   DataSubset c;
   c.regions = std::move(remaining);
   c.merge_regions();
   return c; 
}

bool DataSubset::operator==(const DataSubset& other) const {
   //Making checks in approximate order of cost
   if(start() != other.start()) return false;
   if(regions.empty() != other.regions.empty()) return false;
   if(regions == other.regions) return true;
   if(end() != other.end()) return false;

   size_t a_stride = 0, b_stride = 0;
   for(const auto& r :       regions) if(r.reps) a_stride = r.stride;
   for(const auto& r : other.regions) if(r.reps) b_stride = r.stride;

   //We won't try comparing infinite regions of different strides.
   if(a_stride && b_stride && a_stride != b_stride && end() == MAX)
      return false;

   return (*this - other).empty() && (other - *this).empty();
}

bool DataSubset::operator!=(const DataSubset& other) const {
   return !(*this == other);
}

bool DataSubset::empty() const {
   return regions.empty();
}

std::pair<size_t, size_t> DataSubset::range() const {
   return {start(), end()};
}

size_t DataSubset::start() const {
   if(regions.empty()) return -1;
   return regions.begin()->start;
}

size_t DataSubset::end() const {
   if(empty()) return -1;

   size_t ret = 0;
   for(const auto& r : regions){
      ret = std::max(ret, r.range().second);
   }
   return ret;
}

std::set<DataRegion> DataSubset::bounded_regions(size_t max_idx) const {
   return bounded_regions(0, max_idx);
}

std::set<DataRegion> DataSubset::bounded_regions(
   size_t start, size_t end
) const {
   std::set<DataRegion> ret;
   DataRegion bounds({start, end});
   for(const auto& r : regions){
      if(r.start > end) break;
      if(r.range().second < start) continue;
      ret.merge(r & bounds);
   }
   return ret;
}

size_t DataSubset::count(size_t max_index) const {
   size_t ret = 0;
   for(const auto& r : bounded_regions(max_index)){
      size_t c = r.count();
      if(c == MAX) return 0;
      ret+= c;
   }
   return ret;
}

size_t DataSubset::max_count() const {
   return end()+1;
}

void DataSubset::serialize_data(
   size_t elm_size, const DataBuffer& src, DataBuffer& dst
) const {
   if(regions.empty()){
      dst.resize(0);
      return;
   }
   fenix_assert(src.size()%elm_size == 0);
   const size_t max_elm = src.size()/elm_size-1;

   dst.reset(count(max_elm) * elm_size);
   char* ptr = dst.data();
   
   for(const auto b : BlockIter(bounded_regions(max_elm))){
      size_t start = b.start*elm_size;
      size_t len = (b.end-b.start+1)*elm_size;

      fenix_assert((ptr+len)-dst.data() <= dst.size());
      fenix_assert(start+len <= src.size());

      memcpy(ptr, src.data()+start, len);
      ptr += len;
   }

   fenix_assert(ptr == dst.data()+dst.size());
}

void DataSubset::deserialize_data(
   size_t elm_size, const DataBuffer& src, DataBuffer& dst
) const {
   if(regions.empty()) return;
   fenix_assert(dst.size()%elm_size==0);

   size_t max_elm = dst.size()/elm_size - 1;
   if(max_elm == 0){
      max_elm = end();
      fenix_assert(max_elm != MAX);
      dst.resize((max_elm+1)*elm_size);
   }

   fenix_assert(src.size() == count(max_elm)*elm_size);
   const char* ptr = src.data();

   for(const auto& b : BlockIter(bounded_regions(max_elm))){
      size_t start = b.start*elm_size;
      size_t len = (b.end-b.start+1)*elm_size;

      fenix_assert((ptr+len)-src.data() <= src.size());
      fenix_assert(start+len <= dst.size());

      memcpy(dst.data()+start, ptr, len);
      ptr += len;
   }

   fenix_assert(ptr == src.data()+src.size());
}

void DataSubset::copy_data(
   const size_t elm_size, const size_t src_len, const char* src, DataBuffer& dst
) const {
   if(regions.empty()) return;
   fenix_assert(src_len != 0 || end() != MAX, 
      "must specify either a maximum element count or provide a limited-bounds data subset");

   size_t max_elm = src_len ? src_len-1 : end();
   if(dst.size() < (max_elm+1)*elm_size) dst.resize((max_elm+1)*elm_size);

   for(const auto& b : BlockIter(bounded_regions(max_elm))){
      size_t start = b.start*elm_size;
      size_t len = (b.end-b.start+1)*elm_size;

      fenix_assert(src_len == 0 || (start+len)/elm_size <= src_len);
      fenix_assert(start+len <= dst.size());

      memcpy(dst.data()+start, src+start, len);
   }
}

void DataSubset::copy_data(
   const size_t elm_size, const DataBuffer& src, const size_t dst_len, char* dst
) const {
   if(regions.empty()) return;
   fenix_assert(dst_len != 0 || end() != MAX, 
      "must specify either a maximum element count or provide a limited-bounds data subset");

   size_t max_elm = std::min(dst_len ? dst_len-1 : end(), src.size());

   for(const auto& b : BlockIter(bounded_regions(max_elm))){
      size_t start = b.start*elm_size;
      size_t len = (b.end-b.start+1)*elm_size;

      fenix_assert(dst_len == 0 || (start+len)/elm_size <= dst_len);
      fenix_assert(start+len <= src.size());

      memcpy(dst+start, src.data()+start, len);
   }
}

bool DataSubset::includes(size_t idx) const {
   return !bounded_regions(idx, idx).empty();
}

bool DataSubset::includes_all(size_t end) const {
   std::set<DataRegion> remaining = {DataRegion({0, end})};

   for(const auto& r : regions){
      if(r.start > end) break;

      auto next = remaining.begin();
      while(next != remaining.end()){
         auto it = next++;
         auto rem = *it;

         remaining.erase(it);
         remaining.merge(rem - r);
      }
   }
   
   return remaining.empty();
}

std::string DataSubset::str() const {
    std::string ret = "{";
    for(const auto& r : regions) ret += r.str() + ", ";
    if(!empty()){
        ret.pop_back();
        ret.pop_back();
    }
    ret += "}";
    return ret;
}

} // namespace Fenix

using namespace Fenix;

int __fenix_data_subset_create(
   int num_blocks, int start, int end, int stride, Fenix_Data_subset *subset
) {
  subset->impl = new DataSubset({start, end}, num_blocks, stride);
  return FENIX_SUCCESS;
}

int __fenix_data_subset_createv(
   int num_blocks, int *starts, int *ends, Fenix_Data_subset *subset
) {
   fenix_assert(num_blocks > 0, "num_blocks (%d) must be positive", num_blocks);

   std::vector<std::pair<size_t, size_t>> bounds;
   bounds.reserve(num_blocks);
   for(int i = 0; i < num_blocks; i++) bounds.push_back({starts[i], ends[i]});
   
   subset->impl = new DataSubset(bounds);

   return FENIX_SUCCESS;
}

int __fenix_data_subset_free( Fenix_Data_subset *subset ) {
   delete (DataSubset*) subset->impl;
   return FENIX_SUCCESS;
}
