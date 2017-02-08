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




#include "constants.h"
#include "data_recovery.h"
#include "opt.h"
#include "process_recovery.h"
#include "util.h"
#include "fenix_ext.h"


int _send_group_data(int current_rank, int in_rank, fenix_group_entry_t *gentry,
                     MPI_Comm comm) {
  int i;
  group_entry_packet_t gepacket;
  gepacket.groupid = gentry->groupid;
  gepacket.timestamp = gentry->timestamp;
  gepacket.depth = gentry->depth;
  gepacket.rank_separation = gentry->rank_separation;
  gepacket.state = gentry->state;

  if (__fenix_options.verbose == 67) {
    verbose_print(
            "send c-rank: %d, out-rank: %d, g-id: %d, g-timestamp: %d, g-depth: %d, g-state: %d\n",
            current_rank, in_rank, gentry->groupid, gentry->timestamp, gentry->depth,
            gentry->state);
  }

  MPI_Send(&gepacket, sizeof(group_entry_packet_t), MPI_BYTE, in_rank,
           RECOVER_GROUP_ENTRY_TAG, (gentry->comm)); /* Group entry */

  /* Send a meta data for members  */
  fenix_member_t *member = &(gentry->member);
  two_container_packet_t mpacket;
  mpacket.count = member->count;
  mpacket.size = member->size;

  if (__fenix_options.verbose == 67) {
    verbose_print(
            "send c-rank: %d, inrank: %d,  m-count: %d, m-size: %d\n",
            current_rank, in_rank, member->count, member->size);
  }

  MPI_Send(&mpacket, sizeof(two_container_packet_t), MPI_BYTE, in_rank,
           RECOVER_MEMBER_TAG, gentry->comm); /* Member metadata*/

  return FENIX_SUCCESS;
}

int _recover_group_data(int current_rank, int out_rank, fenix_group_entry_t *gentry,
                        MPI_Comm comm) {

  int i;
  group_entry_packet_t gepacket;
  MPI_Status status;
  /* Receive information from the survived rank */
  MPI_Recv(&gepacket, sizeof(group_entry_packet_t), MPI_BYTE, out_rank,
           RECOVER_GROUP_ENTRY_TAG, comm, &status); /* Group entry */

  gentry->timestamp = gepacket.timestamp;
  gentry->depth = gepacket.depth;
  gentry->rank_separation = gepacket.rank_separation;
  gentry->state = gepacket.state;

  if (__fenix_options.verbose == 68) {
    verbose_print(
            "recv c-rank: %d, p-rank: %d, g-timestamp: %d, g-depth: %d, g-state: %d\n",
            current_rank, out_rank, gentry->groupid,
            gentry->timestamp, gentry->depth,
            gentry->state);
  }

  two_container_packet_t mpacket;
  MPI_Recv(&mpacket, sizeof(two_container_packet_t), MPI_BYTE, out_rank,
           RECOVER_MEMBER_TAG, comm, &status); /* Member metadata */

  fenix_member_t *member = &(gentry->member);

  /* Reinit member entries but no inforation for each entry */
  reinit_member(member, mpacket, NEEDFIX);

  if (__fenix_options.verbose == 68) {
    verbose_print(
            "recv c-rank: %d, p-rank: %d,  m-count: %d, m-size: %d\n",
            current_rank, out_rank, member->count, member->size);
  }
  return FENIX_SUCCESS;
}

int _send_metadata(int current_rank, int out_rank, MPI_Comm comm) {
  /* Implicitly send data to the partner rank */
  fenix_group_t *group = g_data_recovery;
  two_container_packet_t gpacket;
  gpacket.count = group->count;
  gpacket.size = group->size;
  MPI_Send(&gpacket, sizeof(two_container_packet_t), MPI_BYTE, out_rank,
           RECOVER_GROUP_TAG, comm); /* Group metadata */

  if (__fenix_options.verbose == 65) {
    verbose_print(
            "send c-rank: %d, out-rank: %d, g-count: %d, g-size: %d\n",
            current_rank, out_rank,
            group->count, group->size);
  }

  return FENIX_SUCCESS;
}

int _recover_metadata(int current_rank, int in_rank, MPI_Comm comm) {
  MPI_Status status;
  fenix_group_t *group = g_data_recovery;
  two_container_packet_t gpacket;
  MPI_Recv(&gpacket, sizeof(two_container_packet_t), MPI_BYTE, in_rank,
           RECOVER_GROUP_TAG, comm, &status); /* Group metadata */
  group->count = gpacket.count;
  group->size = gpacket.size;

  reinit_group(group, gpacket);

  if (__fenix_options.verbose == 66) {
    verbose_print("recv c-rank: %d, in-rank: %d, g-count: %d, g-size: %d\n",
                  current_rank, in_rank,
                  group->count, group->size);
  }

  return FENIX_SUCCESS;
}

int _pc_send_member_entries(int current_rank, int out_rank, int depth,
                            fenix_version_t *version, MPI_Comm comm) {
  /* Iterate over versions */
  int version_index;
  for (version_index = 0; version_index <
                          version->size; version_index++) { /* used to be version->num_copies */
    /* Make sure the position is decremented by 1 as this is the most recent valid data */
    int remote_entry_offset = (((version->position - 1) + version_index) % depth);
    fenix_remote_entry_t *rentry = &(version->remote_entry[version_index]);
    data_entry_packet_t dpacket;
    dpacket.datatype = rentry->datatype;
    dpacket.count = rentry->count;
    dpacket.size = rentry->size;

    if (__fenix_options.verbose == 73) {
      verbose_print("send version[%d], rd-offset: %d, rd-count: %d, rd-size: %d\n",
                    version_index, remote_entry_offset, rentry->count, rentry->size);
    }

    MPI_Send(&dpacket, sizeof(data_entry_packet_t), MPI_BYTE, out_rank,
             RECOVER_SIZE_TAG + version_index, comm); /* Remote data metadata */

    if (__fenix_options.verbose == 73) {
         int *data = rentry->data; 
         int data_index;
         for (data_index = 0; data_index < rentry->size; data_index++) {
            verbose_print("send version[%d], rd-data[%d]: %d\n", version_index, data_index, data[data_index]);
         }
      }

    /* Send the content of the latest data  */
    if (rentry->count > 0) {
      MPI_Send(rentry->data, rentry->count * rentry->size, MPI_BYTE, out_rank,
               RECOVER_DATA_TAG,
               comm); /* Remote data entry */
      double myd[4];
      memcpy(myd, rentry->data, 32);
      //printf("Recovery data %f %f %f %f\n",myd[0],myd[1],myd[2],myd[3]);
    }

  }  /* end of version_index */
  return FENIX_SUCCESS;
}


int _pc_recover_member_entries(int current_rank, int in_rank, int depth,
                               fenix_version_t *version, MPI_Comm comm) {
  MPI_Status status;
  /* Iterate over versions */
  int version_index;
  for (version_index = 0;
       version_index < version->size; version_index++) { // used to be version->num_copies
    fenix_local_entry_t *lentry = &(version->local_entry[version_index]);
    data_entry_packet_t dpacket;
    MPI_Recv(&dpacket, sizeof(data_entry_packet_t), MPI_BYTE, in_rank,
             RECOVER_SIZE_TAG + version_index, comm, &status); /* Remote data metadata */

    lentry->datatype = dpacket.datatype;
    lentry->count = dpacket.count;
    lentry->size = dpacket.size;
    lentry->data = s_malloc(lentry->size * lentry->count);
    lentry->currentrank = current_rank;

    if (__fenix_options.verbose == 74) {
      verbose_print("recv version[%d], ld-count: %d, ld-size: %d\n",
                    version_index, lentry->count, lentry->size);
    }

    /* Grab remote data entry */
    if (lentry->count > 0) {
      MPI_Recv(lentry->data, lentry->size * lentry->count, MPI_BYTE, in_rank,
               RECOVER_DATA_TAG, comm, &status);
    }

  }
  return FENIX_SUCCESS;
}


int _pc_send_member_metadata(int current_rank, int in_rank,
                             fenix_member_entry_t *mentry, MPI_Comm comm) {
  /* more data needs to be exchanged  */
  member_entry_packet_t mepacket;
  mepacket.memberid = mentry->memberid;
  mepacket.state = mentry->state;
  mepacket.current_datatype = mentry->current_datatype;
  mepacket.size_datatype = mentry->size_datatype;
  mepacket.current_count = mentry->current_count;
  mepacket.current_size = mentry->current_size;
  mepacket.currentrank = mentry->currentrank;
  mepacket.remoterank = mentry->remoterank;

  if (__fenix_options.verbose == 69) {
    verbose_print(
            "send c-rank: %d, p-rank: %d,  m-memberid: %d, m-state: %d\n",
            current_rank, in_rank, mentry->memberid, mentry->state);
  }

  MPI_Send(&mepacket, sizeof(member_entry_packet_t), MPI_BYTE, in_rank,
           RECOVER_MEMBER_ENTRY_TAG, comm); /* Member entry */

  {
    /* Send the meta data of versioning */
    fenix_version_t *version = &(mentry->version);
    container_packet_t vpacket;
    vpacket.count = version->count;
    vpacket.size = version->size;
    vpacket.position = version->position;
    vpacket.num_copies = version->num_copies;

    if (__fenix_options.verbose == 69) {
      verbose_print(
              "send c-rank: %d, p-rank: %d, v-count: %d, v-size: %d, v-pos: %d, v-copies: %d\n",
              current_rank, in_rank, version->count,
              version->size, version->position, version->num_copies);
    }

    MPI_Send(&vpacket, sizeof(container_packet_t), MPI_BYTE, in_rank,
             RECOVERY_VERSION_TAG, comm); /* Version metadata */
  }
  return FENIX_SUCCESS;
}

int _pc_recover_member_metadata(int current_rank, int out_rank,
                                fenix_member_entry_t *mentry, MPI_Comm comm) {
  /* Iterate over members */
  MPI_Status status;
  member_entry_packet_t mepacket;


  MPI_Recv(&mepacket, sizeof(member_entry_packet_t), MPI_BYTE, out_rank,
           RECOVER_MEMBER_ENTRY_TAG, comm, &status); /* Member entry */

  mentry->memberid = mepacket.memberid;
  mentry->state = mepacket.state;
  mentry->current_datatype = mepacket.current_datatype;
  mentry->size_datatype = mepacket.size_datatype;
  mentry->current_count = mepacket.current_count;
  mentry->current_size = mepacket.current_size;
  mentry->currentrank = mepacket.currentrank;
  mentry->remoterank = mepacket.remoterank;

  if (__fenix_options.verbose == 70) {
    verbose_print(
          "send c-rank: %d, p-rank: %d,  m-memberid: %d, m-state: %d\n",
           current_rank, out_rank, mentry->memberid, mentry->state);
  }
  
  
  /* Recover version */
    fenix_version_t *version = &(mentry->version);
    container_packet_t vpacket;

    MPI_Recv(&vpacket, sizeof(container_packet_t), MPI_BYTE, out_rank,
             RECOVERY_VERSION_TAG, comm, &status); /* Version metadata */

    reinit_version(version, vpacket);

  return FENIX_SUCCESS;
}

int _pc_send_members(int current_rank, int out_rank, int depth, fenix_member_t *member,
                     MPI_Comm comm) {
  /* Iterate over members */
  int member_index;
  for (member_index = 0; member_index < member->size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    member_entry_packet_t mepacket;
    mepacket.memberid = mentry->memberid;
    mepacket.state = mentry->state;

    if (__fenix_options.verbose == 71) {
      verbose_print("send c-rank: %d, p-rank: %d, m-memberid: %d, m-state: %d\n",
                    current_rank, out_rank, mentry->memberid, mentry->state);
    }

    MPI_Send(&mepacket, sizeof(member_entry_packet_t), MPI_BYTE, out_rank,
             RECOVER_MEMBER_ENTRY_TAG + member_index, comm); /* Member entry */

    /* Send the meta data of versioning */
    fenix_version_t *version = &(mentry->version);
    container_packet_t vpacket;
    vpacket.count = version->count;
    vpacket.size = version->size;
    vpacket.position = version->position;
    vpacket.num_copies = version->num_copies;

    if (__fenix_options.verbose == 71) {
      verbose_print(
              "send c-rank: %d, p-rank: %d, v-count: %d, v-size: %d, v-pos: %d, v-copies: %d\n",
              current_rank, out_rank, version->count,
              version->size, version->position, version->num_copies);
    }

    MPI_Send(&vpacket, sizeof(container_packet_t), MPI_BYTE, out_rank,
             RECOVERY_VERSION_TAG + member_index, comm); /* Version metadata */

    /* Iterate over versions */
    int version_index;
    for (version_index = 0; version_index <
                            version->size; version_index++) { /* used to be version->num_copies */

      /* Make sure the position is decremented by 1 as this is the most recent valid data */
      int remote_entry_offset = (((version->position - 1) + version_index) % depth);
      fenix_remote_entry_t *rentry = &(version->remote_entry[version_index]);
      data_entry_packet_t dpacket;
      dpacket.datatype = rentry->datatype;
      dpacket.count = rentry->count;
      dpacket.size = rentry->size;

      if (__fenix_options.verbose == 71) {
        verbose_print("send version[%d], rd-offset: %d, rd-count: %d, rd-size: %d\n",
                      version_index, remote_entry_offset, rentry->count, rentry->size);
      }

      MPI_Send(&dpacket, sizeof(data_entry_packet_t), MPI_BYTE, out_rank,
               RECOVER_SIZE_TAG + version_index, comm); /* Remote data metadata */

      if (__fenix_options.verbose == 71) {
         int *data = rentry->data; 
         int data_index;
         for (data_index = 0; data_index < rentry->size; data_index++) {
            verbose_print("send version[%d], rd-data[%d]: %d\n", version_index, data_index, data[data_index]);
         }
      }

      /* Send the content of the latest data  */
      MPI_Send(rentry->data, rentry->size, MPI_BYTE, out_rank,
               RECOVER_DATA_TAG + version_index,
               comm); /* Remote data entry */

    } /* end of version_index */
  } /* end of member_index */
  return FENIX_SUCCESS;
}


int _pc_recover_members(int current_rank, int in_rank, int depth, fenix_member_t *member,
                        MPI_Comm comm) {
  MPI_Status status;
  int member_index;
  for (member_index = 0; member_index < member->size; member_index++) {
    fenix_member_entry_t *mentry = &(member->member_entry[member_index]);
    member_entry_packet_t mepacket;
    MPI_Recv(&mepacket, sizeof(member_entry_packet_t), MPI_BYTE, in_rank,
             RECOVER_MEMBER_ENTRY_TAG + member_index, comm, &status); /* Member entry */
    mentry->memberid = mepacket.memberid;
    mentry->state = mepacket.state;

    if (__fenix_options.verbose == 72) {
      verbose_print(
              "recv c-rank: %d, p-rank: %d, m-memberid: %d, m-state: %d\n",
              current_rank, in_rank, mentry->memberid, mentry->state);
    }

    fenix_version_t *version = &(mentry->version);
    container_packet_t vpacket;

    MPI_Recv(&vpacket, sizeof(container_packet_t), MPI_BYTE, in_rank,
             RECOVERY_VERSION_TAG + member_index, comm, &status); /* Version metadata */

    reinit_version(version, vpacket);

    if (__fenix_options.verbose == 72) {
      verbose_print(
              "recv c-rank: %d, p-rank: %d, v-count: %d, v-size: %d, v-pos: %d, v-copies: %d\n",
              current_rank, in_rank, version->count, version->size,
              version->position, version->num_copies);
    }


    /* Iterate over versions */
    int version_index;
    for (version_index = 0; version_index <
                            version->size; version_index++) { // used to be version->num_copies
      int local_entry_offset = (((version->position - 1) + version_index) % depth);
      fenix_local_entry_t *lentry = &(version->local_entry[version_index]);
      data_entry_packet_t dpacket;
      MPI_Recv(&dpacket, sizeof(data_entry_packet_t), MPI_BYTE, in_rank,
               RECOVER_SIZE_TAG + version_index, comm,
               &status); /* Remote data metadata */

      lentry->datatype = dpacket.datatype;
      lentry->count = dpacket.count;
      lentry->size = dpacket.size;
      lentry->data = s_malloc(lentry->size * lentry->count);
      lentry->currentrank = current_rank;

      if (__fenix_options.verbose == 72) {
        verbose_print("recv version[%d], ld-offset: %d, ld-count: %d, ld-size: %d\n",
                      version_index, local_entry_offset, lentry->count, lentry->size);
      }

      /* Grab remote data entry */
      MPI_Recv(lentry->data, lentry->size, MPI_BYTE, in_rank,
               RECOVER_DATA_TAG + version_index,
               comm, &status);

    } /* end of version_index */
  } /* end of member_index */
  return FENIX_SUCCESS;
}

