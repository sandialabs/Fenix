fail_list
=========

.. operation:: local

Get the list of failed rank IDs from the most recent failure.

.. c:function:: int Fenix_Process_fail_list(int** fail_list)

   :param int** fail_list: [out] Pointer to pointer that will be set to a dynamically allocated array of failed rank IDs from the most recent failure. Caller is responsible for freeing this memory with free(). Array length equals the number of failed ranks.
   :returns: Number of failed ranks if successful, or error code if negative

.. cpp:function:: std::vector<int> fenix::fail_list()

   :returns: Vector of failed rank IDs

.. note::
   The C++ overload returns a std::vector, eliminating manual memory management.

**Return Codes:**

- **Positive value (>= 0)** - Number of failed ranks from the most recent failure
- :c:enumerator:`FENIX_ERROR_UNINITIALIZED` - Fenix has not been initialized yet

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
