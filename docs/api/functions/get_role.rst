get_role
========

.. operation:: local

Query the role of the current rank.

.. c:function:: int Fenix_get_rank_role(MPI_Comm comm, int rank, int* role)

   :param MPI_Comm comm: Communicator to query
   :param int rank: Rank to query
   :param int* role: The rank's role
   :returns: FENIX_SUCCESS if successful

.. cpp:function:: Role fenix::get_role()

   :returns: The current rank's :c:type:`Fenix_Rank_role`

.. note::
   The C++ overload queries the current rank's role directly without needing comm or rank parameters.

.. seealso::
   :c:type:`Fenix_Rank_role`, :c:func:`Fenix_Init`
