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
#ifndef __FENIX_DATA_SUBSET_HPP__
#define __FENIX_DATA_SUBSET_HPP__

#include "fenix.h"
#include "fenix_opt.hpp"
#include "fenix_data_buffer.hpp"

#include <utility>
#include <set>
#include <optional>
#include <limits>
#include <string>

namespace Fenix {

namespace Detail {

struct DataRegionIterator;

struct DataRegion {
    static constexpr size_t MAX = std::numeric_limits<size_t>::max();

    DataRegion(std::pair<size_t, size_t> b)
        : DataRegion(b, 0, MAX) { };
    DataRegion(std::pair<size_t, size_t> b, size_t m_reps, size_t m_stride)
        : start(b.first),
          end((b.second!=MAX && b.second+1==b.first+m_stride) ?
                  b.second+m_reps*m_stride : b.second),
          reps((b.second==MAX || b.second+1==b.first+m_stride) ? 0 : m_reps),
          stride(reps == 0 ? MAX : m_stride)
    {
        fenix_assert(start <= end);
        fenix_assert(stride != MAX || reps == 0);
        fenix_assert(reps == 0 || start+stride > end);
    };

    //Overall range of this region.
    std::pair<size_t, size_t> range() const;

    //Count of elements contained in this region
    size_t count() const;

    bool operator==(const DataRegion& other) const;
    bool operator!=(const DataRegion& other) const;

    //Order based on start
    bool operator<(const DataRegion& other) const;

    //Return true if these regions intersect
    bool operator&&(const DataRegion& other) const;

    //Returns intersection of two regions. Not defined when both regions are strided.
    std::set<DataRegion> operator&(const DataRegion& other) const;
    
    //This region w/o the overlap with other
    std::set<DataRegion> operator-(const DataRegion& other) const;

    //Get a region that is a single repetition of this one's, with bounds check
    DataRegion get_rep(size_t n) const;
    //As above, but multiple repetitions
    DataRegion get_reps(size_t first, size_t last) const;

    //For a strided region, returns region between repetitions
    //Undefined for an unstrided region.
    DataRegion inverted() const;

    std::optional<DataRegion> try_merge(const DataRegion& other) const;

    std::string str() const;

    //Inclusive region bounds.
    size_t start, end;

    //Number of times to repeat this after the first
    size_t reps;

    //Distance between starts of each repetition
    size_t stride;
};
} // namespace Detail

struct DataSubset {
    static constexpr size_t MAX = Detail::DataRegion::MAX;

    //DataSubset(const DataSubset&) = default;
    //DataSubset(DataSubset&&) = default;
    //Empty
    DataSubset() = default;
    //[0, end]
    explicit DataSubset(size_t end);
    //[bounds.first, bounds.second]
    DataSubset(std::pair<size_t, size_t> bounds);
    //[b.first, b.second], ..., [b.first+stride*(n-1), b.second+stride*(n-1)]
    DataSubset(std::pair<size_t, size_t> b, size_t n, size_t stride);
    //[bounds[0].first, bounds[0].second], ...
    DataSubset(std::vector<std::pair<size_t, size_t>> bounds);
    //Merge two subsets
    DataSubset(const DataSubset& a, const DataSubset& b);
    //Create from serialized subset object
    DataSubset(const DataBuffer& buf);

    DataSubset operator+(const DataSubset& other) const;
    DataSubset& operator+=(const DataSubset& other);
    
    DataSubset operator+(const Fenix_Data_subset& other) const;
    DataSubset& operator+=(const Fenix_Data_subset& other);

    DataSubset operator-(const DataSubset& other) const;
    bool operator==(const DataSubset& other) const;
    bool operator!=(const DataSubset& other) const;

    bool empty() const;

    //Overall range of this subset
    std::pair<size_t, size_t> range() const;
    //Equivalent to range().first and range().second, possibly more performant
    size_t start() const;
    size_t end() const;

    //Count of elements in this subset from [0, max_index]
    //Returns 0 if max_index==end()==MAX
    size_t count(size_t max_index) const;

    //Count of elements in this subset if it were full [0, end()]
    //Returns 0 if end()==MAX
    size_t max_count() const;

    //Serialize this subset object into buf
    //Will resize buf to fit exactly.
    void serialize(DataBuffer& buf) const;
   
    //Will reset dst to fit
    void serialize_data(
        size_t elm_size, const DataBuffer& src, DataBuffer& dst
    ) const;
    //If dst.size()==0, will resize dst to fit
    void deserialize_data(
        size_t elm_size, const DataBuffer& src, DataBuffer& dst
    ) const;

    //If src_len == 0, will assume src is a large as needed
    //Will resize dst if too small
    void copy_data(
        const size_t elm_size, const size_t src_len, const char* src, DataBuffer& dst
    ) const;
    //If dst_len == 0, will assume dst is as large as needed
    void copy_data(
        const size_t elm_size, const DataBuffer& src, const size_t dst_len,
        char* dst
    ) const;
    
    //Whether this subset includes the element at index idx
    bool includes(size_t idx) const;
    //Whether this subset includes the entire range [0, end] without gaps
    bool includes_all(size_t end) const;

    //Return equivalent of regions & [0, max_index]
    std::set<Detail::DataRegion> bounded_regions(size_t max_index) const;
    //As above, but regions & [start, end]
    std::set<Detail::DataRegion> bounded_regions(size_t start, size_t end) const;

    std::string str() const;

    //Individual data regions in this subset
    std::set<Detail::DataRegion> regions;

  private:
    //merge immediately adjacent regions to simplify
    void merge_regions();
};
} // namespace Fenix
#endif // __FENIX_DATA_SUBSET_HPP_
