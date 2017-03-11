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
#include "fenix_hash.h"

/**
 * @brief
 * @param table 
 */
void __fenix_hash_table_print(const struct __fenix_hash_table *table) {
    int i; 
    for(i = 0; i < table->size; i++) {
        if(table->table[i].state == OCCUPIED) {
            MPI_Request *value = __fenix_hash_table_get(table, table->table[i].key);
            printf("[%d] %lu --> %d\n", i, table->table[i].key, (long) value); 
        }
    }
}

/**
 * @brief
 * @param key 
 */
int __fenix_hash (long key) {
  key = (~key) + (key << 18);
  key = key ^ (key >> 31);
  key = key * 21; 
  key = key ^ (key >> 11);
  key = key + (key << 6);
  key = key ^ (key >> 22);
  return (int) key;
}

/**
 * @brief
 * @param table
 */
long *__fenix_keys(const struct __fenix_hash_table *table) {

    int i, n_occupied = 0, n_inserted = 0;
    long *list_of_keys;
    for (i = 0; i < table->size; i++) {
        if(table->table[i].state == OCCUPIED) {
            n_occupied++;
        }
    }

    list_of_keys = malloc(sizeof(long) * n_occupied);
    for (i = 0; i < table->size; i++) { 
        if(table->table[i].state == OCCUPIED) {
            list_of_keys[n_inserted++] = table->table[i].key;
        }
    }
    list_of_keys[n_inserted] = 0;
    return list_of_keys;
}

/**
 * @brief
 * @param  
 */
struct __fenix_hash_table *__fenix_hash_table_new(size_t size) {
    int i;
    struct __fenix_hash_table *table = calloc(1, sizeof(struct __fenix_hash_table));

    table->size = size;
    table->table = malloc(sizeof(__fenix_pair) * size);

    for(i = 0; i < size; i++) {
        table->table[i].key = 0;
        table->table[i].value= 0;
        table->table[i].state = EMPTY;
    }

    if(!table->table) {
        return 0;
    }
    
    table->count = 0;
    return table;
}

/**
 * @brief
 * @param  
 * @param  
 */
MPI_Request *__fenix_hash_table_get(const struct __fenix_hash_table *table, long key) {

    long hash_code = __fenix_hash(key) % table->size;
    long index = hash_code;

    do {
        __fenix_pair *list = table->table;
        switch (list[index].state)
        {
            case EMPTY: 
                         return 0;
                         break;

            case OCCUPIED:
                        if(list[index].key == key) {
                            return list[index].value;
                        }
                        break;

            case DELETED:
                        if(list[index].key == key) {
                            return  0;
                        }
                        break;
        }
        index = (index + 1) % table->size;
    } while (index != hash_code);

    return 0;
}

/**
 * @brief
 * @param  
 */
void __fenix_hash_table_destroy (struct __fenix_hash_table *table) {
    if(table) {
        int i;
        for (i = 0; i < table->size; i++) {
            if (table->table[i].state == OCCUPIED) {
                table->table[i].key = 0;
                table->table[i].value = 0;
            } else if (table->table[i].state == DELETED) {
                table->table[i].key = 0;
            }
        }
        free(table->table);
        free(table);
    }
}

/**
 * @brief
 * @param  
 * @param  
 * @param  
 */
int __fenix_hash_table_put(struct __fenix_hash_table *table, long key, MPI_Request *value) {

    long hash_code = __fenix_hash(key) % table->size;
    long index = hash_code;

    do {
        __fenix_pair *list = table->table;
        switch(list[index].state) {
            case EMPTY:
            case DELETED: 
                list[index].key = key;
                list[index].value = value;
                list[index].state = OCCUPIED;
                table->count++;
                return 0;
                break;
            case OCCUPIED: 
                if(list[index].key == key) {
                    list[index].value = value;
                    return 0;
                } else {
                    index = (index + 1) % table->size;
                }
                break;
        }
    } while (index != hash_code);
    return 0;
}

/**
 * @brief
 * @param  
 * @param  
 */
MPI_Request *__fenix_hash_table_remove(struct __fenix_hash_table *table, long key) {
    long hash_code = __fenix_hash(key) % table->size;
    long index = hash_code;
    __fenix_pair *list = table->table;

    do {
        switch(list[index].state) {
            case EMPTY:
                return 0;
                break;
            case DELETED:
                if (list[index].key == key) {
                    return 0;
                }
            case OCCUPIED:
                if (list[index].key == key) {
                    MPI_Request *value = list[index].value;
                    list[index].value = 0; 
                    list[index].state = DELETED;
                    table->count--;
                    return value;
                }
        }
        index = (index + 1) % table->size;
    } while(index != hash_code);
    return 0;
}
