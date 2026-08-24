#include <fenix.hpp>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>

constexpr int my_group  = 0;
constexpr int my_member = 0;

using fenix::DataSubset;
using namespace fenix::data;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  MPI_Comm res_comm;
  fenix::init({.out_comm = &res_comm});

  int num_ranks, rank;
  MPI_Comm_size(res_comm, &num_ranks);
  MPI_Comm_rank(res_comm, &rank);

  group_create(my_group, {.depth = 2});

  std::vector<int> data;

  if (rank == 0) fprintf(stderr, "Checkpoint initial data, 1/3\n");
  data.resize(30);
  for (int& i : data) i = 0xAAAAAAAA;
  member_define(my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT);
  checkpoint(my_group, DataSubset({0, 29}));

  if (rank == 0) fprintf(stderr, "Checkpoint initial data, 2/3\n");
  data.resize(20);
  for (int& i : data) i = 0;
  member_define(my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT);
  checkpoint(my_group, DataSubset({0, 19}));

  if (rank == 0) fprintf(stderr, "Checkpoint initial data, 3/3\n");
  data.resize(10);
  for (int& i : data) i = 0x55555555;
  member_define(my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT);
  checkpoint(my_group, DataSubset({0, 9}));

  if (rank == 0) fprintf(stderr, "Begin testing loading snapshots\n");
  std::vector<int> ts = *group_snapshots(my_group);
  fenix_require(ts.size() == 3);
  data.resize(40);
  for (int& i : data) i = -1;
  member_define(my_group, my_member, data.data(), FENIX_RESIZEABLE, MPI_INT);

  if (rank == 0) fprintf(stderr, "Test loading individual snapshots\n");
  member_load(my_group, my_member, ts[2]);
  for (int i = 0; i < 40; i++) {
    if (i < 30) fenix_require(data[i] == 0xAAAAAAAA);
    else fenix_require(data[i] == -1);
  }

  member_load(my_group, my_member, ts[1]);
  for (int i = 0; i < 40; i++) {
    if (i < 20) fenix_require(data[i] == 0);
    else if (i < 30) fenix_require(data[i] == 0xAAAAAAAA);
    else fenix_require(data[i] == -1);
  }

  member_load(my_group, my_member, ts[0]);
  for (int i = 0; i < 40; i++) {
    if (i < 10) fenix_require(data[i] == 0x55555555);
    else if (i < 20) fenix_require(data[i] == 0);
    else if (i < 30) fenix_require(data[i] == 0xAAAAAAAA);
    else fenix_require(data[i] == -1);
  }

  if (rank == 0) fprintf(stderr, "Test loading all snapshots\n");
  for (int& i : data) i = -1;
  member_load(my_group, my_member, FENIX_DATA_SNAPSHOT_ALL);
  for (int i = 0; i < 40; i++) {
    if (i < 10) fenix_require(data[i] == 0x55555555);
    else if (i < 20) fenix_require(data[i] == 0);
    else if (i < 30) fenix_require(data[i] == 0xAAAAAAAA);
    else fenix_require(data[i] == -1);
  }

  if (rank == 0) fprintf(stderr, "Test loading latest snapshot\n");
  for (int& i : data) i = -1;
  member_load(my_group, my_member, FENIX_DATA_SNAPSHOT_LATEST);
  for (int i = 0; i < 40; i++) {
    if (i < 10) fenix_require(data[i] == 0x55555555);
    else fenix_require(data[i] == -1);
  }

  if (rank == 0) fprintf(stderr, "Test loading to another location\n");
  member_define(my_group, my_member, nullptr, FENIX_RESIZEABLE, MPI_INT);
  for (int& i : data) i = -1;
  member_load(my_group, my_member, data.data(), 30);
  for (int i = 0; i < 40; i++) {
    if (i < 10) fenix_require(data[i] == 0x55555555);
    else if (i < 20) fenix_require(data[i] == 0);
    else if (i < 30) fenix_require(data[i] == 0xAAAAAAAA);
    else fenix_require(data[i] == -1);
  }

  if (rank == 0) fprintf(stderr, "Test loading to FENIX_DATA_RESTORE_FULL\n");
  for (int& i : data) i = -1;
  member_load(my_group, my_member, data.data());
  for (int i = 0; i < 40; i++) {
    if (i < 10) fenix_require(data[i] == 0x55555555);
    else if (i < 20) fenix_require(data[i] == 0);
    else if (i < 30) fenix_require(data[i] == 0xAAAAAAAA);
    else fenix_require(data[i] == -1);
  }

  if (rank == 0) fprintf(stderr, "Test file load_begin/end\n");
  for (int& i : data) i = -1;
  FILE* fp;
  member_load_begin(my_group, my_member, &fp, ts[1]);
  fread(data.data(), sizeof(int), 20, fp);
  member_load_end(my_group, my_member);
  for (int i = 0; i < 40; i++) {
    if (i < 20) fenix_require(data[i] == 0);
    else fenix_require(data[i] == -1);
  }

  if (rank == 0) fprintf(stderr, "Test stream load_begin/end\n");
  for (int& i : data) i = -1;
  std::iostream* strm;
  member_load_begin(my_group, my_member, &strm, ts[1]);
  strm->read((char*)data.data(), sizeof(int) * 20);
  member_load_end(my_group, my_member);
  for (int i = 0; i < 40; i++) {
    if (i < 20) fenix_require(data[i] == 0);
    else fenix_require(data[i] == -1);
  }

  Fenix_Finalize();
  MPI_Finalize();
}
