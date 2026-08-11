mlog_activate
=============

.. operation:: local

Activate message logging for a communicator.

.. c:function:: int Fenix_Mlog_activate(int mlog_id)

   :param int mlog_id: [in] The message log identifier to activate. Must be a valid log created with :c:func:`Fenix_Mlog_create`. After activation, MPI operations on this communicator will be logged for potential replay.
   :returns: FENIX_SUCCESS if successful, error code if mlog_id invalid

**Return Codes:**

- :c:enumerator:`FENIX_SUCCESS` - Message log activated successfully
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized with :c:func:`Fenix_Init`
- :c:enumerator:`FENIX_ERROR_INVALID_MLOGID` - The specified mlog_id does not exist (was not created with :c:func:`Fenix_Mlog_create`)

**Usage Example:**

.. code-block:: c

   // Basic message logging setup and activation
   int main(int argc, char** argv) {
       int role, error;
       MPI_Comm fenix_comm;

       MPI_Init(&argc, &argv);
       Fenix_Init(&role, MPI_COMM_WORLD, &fenix_comm, &argc, &argv, 2, &error);

       // Create message log with depth of 20 messages
       int mlog_id = 0;
       int depth = 20;
       error = Fenix_Mlog_create(mlog_id, &fenix_comm, depth);
       if (error != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to create message log\n");
           return 1;
       }

       // Activate logging before MPI operations
       error = Fenix_Mlog_activate(mlog_id);
       if (error != FENIX_SUCCESS) {
           fprintf(stderr, "Failed to activate message log\n");
           return 1;
       }

       // Now MPI operations on fenix_comm will be logged
       int rank, size;
       MPI_Comm_rank(fenix_comm, &rank);
       MPI_Comm_size(fenix_comm, &size);

       // These operations are logged and can be replayed on recovery
       int send_data = rank * 100;
       int recv_data = 0;

       if (rank < size - 1) {
           MPI_Send(&send_data, 1, MPI_INT, rank + 1, 0, fenix_comm);
       }
       if (rank > 0) {
           MPI_Recv(&recv_data, 1, MPI_INT, rank - 1, 0, fenix_comm, MPI_STATUS_IGNORE);
       }

       // Deactivate when done with this communication phase
       Fenix_Mlog_deactivate();

       Fenix_Finalize();
       MPI_Finalize();
       return 0;
   }

.. note::
   Message logging is typically activated before critical MPI operations and deactivated afterward to control memory usage. Logs consume memory proportional to the message size and depth parameter.

.. seealso::
   :c:func:`Fenix_Mlog_create`, :c:func:`Fenix_Mlog_deactivate`
