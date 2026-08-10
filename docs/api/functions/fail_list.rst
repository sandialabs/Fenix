fail_list
=========

.. operation:: local

Get the list of failed rank IDs from the most recent failure.

.. c:function:: int Fenix_Process_fail_list(int** fail_list)

   :param int** fail_list: Pointer to array of failed rank IDs. Caller must free this memory.
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: std::vector<int> fenix::fail_list()

   :returns: Vector of failed rank IDs

.. note::
   The C++ overload returns a std::vector, eliminating manual memory management.

.. code-block:: c

   // C example
   int* failed_ranks;
   Fenix_Process_fail_list(&failed_ranks);
   // Use failed_ranks
   free(failed_ranks);

.. code-block:: cpp

   // C++ example
   std::vector<int> failed_ranks = fenix::fail_list();
   for (int rank : failed_ranks) {
       std::cout << "Rank " << rank << " failed\n";
   }
