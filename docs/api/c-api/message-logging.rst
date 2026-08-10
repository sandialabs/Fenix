Message Logging
===============

Message logging provides automatic replay of MPI messages for localized fault tolerance.

Creation and Activation
------------------------

.. c:function:: int Fenix_Mlog_create(int mlog_id, MPI_Comm* comm, int depth)

   Create a message log for a communicator.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier
   :param comm: [in] Communicator to log
   :param depth: [in] Log depth

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Mlog_activate(int mlog_id)

   Activate a message log to begin logging operations.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Mlog_active(int* mlog_id)

   Query the currently active message log.

   .. rubric:: Local Operation

   :param mlog_id: [out] Active message log identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Mlog_delete(int mlog_id)

   Delete a message log and free resources.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Region Management
-----------------

.. c:function:: int Fenix_Mlog_begin_region(int mlog_id, int region_id)

   Begin a logging region.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier
   :param region_id: [in] Region identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Mlog_activate_region(int mlog_id, int region_id)

   Activate logging for a specific region.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier
   :param region_id: [in] Region identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Mlog_sync(int mlog_id, int region_id)

   Synchronize message logs across ranks.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier
   :param region_id: [in] Region identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

Data Member Integration
-----------------------

.. c:function:: int Fenix_Mlog_create_data_member(int mlog_id, int group_id, int member_id)

   Associate a data member with message logging.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier
   :param group_id: [in] Data group identifier
   :param member_id: [in] Data member identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise

.. c:function:: int Fenix_Mlog_define_data_member(int mlog_id, int group_id, int member_id)

   Define a data member for message logging without creating storage.

   .. rubric:: Collective Operation

   :param mlog_id: [in] Message log identifier
   :param group_id: [in] Data group identifier
   :param member_id: [in] Data member identifier

   :returns: FENIX_SUCCESS if successful, any :ref:`return code <return-codes>` otherwise
