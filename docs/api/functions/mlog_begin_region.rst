mlog_begin_region
=================

.. operation:: local

Set the region of a message logger.

.. c:function:: int Fenix_Mlog_begin_region(int mlog_id, int region_id)

   :param int mlog_id: [in] The message log identifier whose region to update. Must be a valid existing log.
   :param int region_id: [in] The new region ID to set. Must be positive and greater than current region_id. May equal current region_id only if no messages have been logged in that region yet. Regions help organize message logging for structured recovery.
   :returns: Error code, see below

.. cpp:function:: int fenix::mlog::begin_region(int mlog_id, int region_id)

   :param int mlog_id: [in] The logger to set the region of
   :param int region_id: [in] The region ID to set (must be positive and > current)
   :returns: Error code, see below

.. note::
   Region ID must be positive and greater than current region_id (may equal current region_id
   if no messages have been logged in the region).

Return Codes
------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Return Code
     - Condition
   * - :c:enumerator:`FENIX_SUCCESS`
     - Region successfully set
   * - :c:enumerator:`FENIX_ERROR_UNINITIALIZED`
     - Fenix has not been initialized via :c:func:`Fenix_Init`
   * - :c:enumerator:`FENIX_ERROR_INVALID_MLOGID`
     - The specified ``mlog_id`` does not correspond to an existing message logger

.. warning::
   If ``region_id`` is less than the current region and messages have already been logged
   in the current region, or if ``region_id`` equals the current region and messages have
   already been logged, the function will abort execution with a fatal error rather than
   return an error code. These represent programming errors that cannot be recovered.

.. seealso::
   :c:func:`Fenix_Mlog_activate_region`, :c:func:`Fenix_Mlog_sync`

Example
-------

Message logging regions help organize fault-tolerant iteration loops. Each region captures
all MPI messages within that phase of computation, allowing recovery to replay messages from
a specific checkpoint point.

**C Example:**

.. code-block:: c

   #include <fenix.h>
   #include <mpi.h>

   int main(int argc, char** argv) {
       MPI_Init(&argc, &argv);

       MPI_Comm fenix_comm;
       Fenix_Init(&fenix_comm, MPI_COMM_WORLD, NULL, NULL, 0, 0, 1, 1, MPI_INFO_NULL, &error);

       int rank, size;
       MPI_Comm_rank(fenix_comm, &rank);
       MPI_Comm_size(fenix_comm, &size);

       // Create message logger with capacity for 10 regions
       int mlog_id;
       Fenix_Mlog_create(mlog_id, fenix_comm, 10);
       Fenix_Mlog_activate(mlog_id);

       // Create data group for checkpointing
       int group_id = 0;
       Fenix_Data_group_create(group_id, fenix_comm, 0, 1, FENIX_DATA_POLICY_IN_MEMORY_RAID, NULL, &error);

       // Application state
       double local_data[100];
       int iteration = 0;

       // Main computation loop
       for (iteration = 0; iteration < 1000; iteration++) {
           // Start a new message logging region for this iteration
           Fenix_Mlog_begin_region(mlog_id, iteration);

           // All MPI communication in this region will be logged
           int left_neighbor = (rank - 1 + size) % size;
           int right_neighbor = (rank + 1) % size;

           double recv_left, recv_right;
           MPI_Sendrecv(&local_data[0], 1, MPI_DOUBLE, left_neighbor, 0,
                        &recv_right, 1, MPI_DOUBLE, right_neighbor, 0,
                        fenix_comm, MPI_STATUS_IGNORE);

           // Compute using received data
           for (int i = 0; i < 100; i++) {
               local_data[i] = 0.5 * (local_data[i] + recv_left + recv_right);
           }

           // Checkpoint every 10 iterations
           if (iteration % 10 == 0) {
               Fenix_Data_member_store(group_id, 0, FENIX_SUBSET_FULL);
               Fenix_Data_commit_barrier(group_id);
           }
       }

       Fenix_Finalize();
       MPI_Finalize();
       return 0;
   }

**C++ Example:**

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <vector>

   int main(int argc, char** argv) {
       MPI_Init(&argc, &argv);

       MPI_Comm fenix_comm;
       fenix::init({.out_comm = &fenix_comm, .spares = 2});

       int rank = fenix::rank();
       int size = fenix::size();

       // Create message logger
       constexpr int mlog_id = 0;
       fenix::mlog::create(mlog_id, fenix_comm, 10);
       fenix::mlog::activate(mlog_id);

       std::vector<double> data(100, 0.0);

       // Computation loop with regions
       for (int iteration = 0; iteration < 1000; iteration++) {
           // Begin new region - messages will be logged under this region ID
           fenix::mlog::begin_region(mlog_id, iteration);

           // Perform MPI communication and computation
           int neighbor = (rank + 1) % size;
           double send_val = data[0];
           double recv_val;

           MPI_Sendrecv(&send_val, 1, MPI_DOUBLE, neighbor, 0,
                        &recv_val, 1, MPI_DOUBLE, neighbor, 0,
                        fenix_comm, MPI_STATUS_IGNORE);

           // Update local data
           for (auto& val : data) {
               val = 0.5 * (val + recv_val);
           }
       }

       fenix::finalize();
       MPI_Finalize();
       return 0;
   }

The key pattern is: call ``Fenix_Mlog_begin_region`` with an increasing region ID at the
start of each iteration or phase. This allows Fenix to organize logged messages chronologically
and replay only the messages from the correct checkpoint point during recovery.
