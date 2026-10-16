group_cohort
============

.. operation:: local

Query the cohort (redundancy partner group) for a data group.

.. c:function:: int Fenix_Data_group_get_cohort(int group_id, MPI_Group* cohort)

   :param int group_id: The data group to query.
   :param MPI_Group* cohort: Pointer to the returned MPI group.
   :returns: FENIX_SUCCESS if successful, error code otherwise

.. cpp:function:: MPI_Group fenix::data::group_cohort(int group_id)

   :param int group_id: The data group to query
   :returns: The cohort MPI group

The cohort is the set of ranks that participate in backing up each other's
member data for the group. It includes the calling rank and all partner ranks.
For local-only policies with no redundancy, the cohort contains only the
calling rank.

.. note::
   The returned MPI group is a duplicate. The caller is responsible for freeing
   it with ``MPI_Group_free`` when it is no longer needed.

.. seealso::
   :c:func:`Fenix_Data_group_create`, :c:func:`Fenix_Data_group_get_redundancy_policy`, :cpp:func:`fenix::data::group_create`
