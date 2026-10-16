fail_list
=========

.. operation:: local

Get the list of failed rank IDs from the most recent failure.

.. c:function:: int Fenix_Process_fail_list(int** fail_list)

   :param int** fail_list: Pointer to pointer that will be set to an array of failed rank IDs from the most recent failure. Caller does not own the memory allocation, and should not free it. Array length equals the number of failed ranks.
   :returns: Number of failed ranks if successful, or error code if negative

.. cpp:function:: std::vector<int> fenix::fail_list()

   :returns: Vector of failed rank IDs
