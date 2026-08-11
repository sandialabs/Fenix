Examples
========

Example programs demonstrating Fenix usage are in the ``examples/`` directory.

.. important::
   **Start with Example 8!** It demonstrates the modern, recommended API patterns.

Recommended Learning Path
-------------------------

1. **Example 8 (Start Here!)** - Modern inline recovery with message logging ⭐
2. **Example 7** - Resizable data members with modern API
3. **Examples 1-4** - Simpler patterns (older API, will be updated)
4. **Examples 5-6** - Data subset operations

.. toctree::
   :maxdepth: 1
   :caption: Example Walkthroughs:

   08-inline-recovery
   07-resizeable-member
   01-hello-world

Example Overview
----------------

Example 8: Inline Recovery (RECOMMENDED ⭐)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Modern C++ API, Inline Recovery, Message Logging**

This is the gold standard example showing production-ready patterns:

- Modern ``fenix::init()`` with designated initializers
- Exception-based inline recovery
- Recovery callbacks for seamless continuation
- Automatic message logging and replay
- Handles multiple failures gracefully

:doc:`View detailed walkthrough → <08-inline-recovery>`

**Source:** ``examples/08_inline_recovery/stencil_skeleton.cpp``

Example 7: Resizable Members
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Modern C++ API, Dynamic Data Structures**

Demonstrates working with resizable data (e.g., ``std::vector``):

- Using ``FENIX_RESIZEABLE`` data members
- Updating buffer pointers after resize
- Querying stored subset sizes with null restore
- Modern exception-based recovery

:doc:`View detailed walkthrough → <07-resizeable-member>`

**Source:** ``examples/07_resizeable_member/resizeable.cpp``

Example 6: Vector-based Subsets
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Data Subset Operations**

Shows how to checkpoint only portions of arrays:

- Creating subsets with multiple blocks using ``Fenix_Data_subset_createv``
- Storing and restoring partial data
- Useful for large arrays where full checkpoint is expensive

**Source:** ``examples/06_subset_createv/subset_createv.c``

Example 5: Subset Create
~~~~~~~~~~~~~~~~~~~~~~~~~

**Data Subset Operations**

Basic subset operations for partial checkpoints.

**Source:** ``examples/05_subset_create/subset_create.c``

Example 4: Non-Blocking Communication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Isend/Irecv with Fault Tolerance**

Demonstrates non-blocking MPI operations (``MPI_Isend``, ``MPI_Irecv``) with fault tolerance.

.. note::
   This example uses older API patterns. See :doc:`08-inline-recovery` for modern patterns.

**Source:** ``examples/04_Isend_Irecv/fenix/fenix_stencil_1D.c``

Example 3: Collective Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Reduce with Fault Tolerance**

Shows collective operations (``MPI_Reduce``) surviving failures.

.. note::
   This example uses older API patterns. See :doc:`08-inline-recovery` for modern patterns.

**Source:** ``examples/03_reduce/fenix/fenix_reduce.c``

Example 2: Point-to-Point Communication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Send/Recv with Data Recovery**

Ring communication pattern with data checkpointing.

.. note::
   This example uses older API patterns. See :doc:`08-inline-recovery` for modern patterns.

**Source:** ``examples/02_send_recv/fenix/fenix_ring.c``

Example 1: Hello World
~~~~~~~~~~~~~~~~~~~~~~

**Basic Initialization**

:doc:`Walkthrough available → <01-hello-world>`

Minimal fault-tolerant MPI program showing basic initialization and process recovery.

.. note::
   This example uses older API patterns. See :doc:`08-inline-recovery` for modern patterns.

**Source:** ``examples/01_hello_world/fenix/fenix_hello_world.c``

Running the Examples
--------------------

All examples are built when you configure with ``-DBUILD_EXAMPLES=ON``:

.. code-block:: bash

   cd Fenix/build/examples
   ls  # See all example directories

   # Run Example 8 (recommended)
   cd 08_inline_recovery
   mpiexec --with-ft mpi -n 7 ./stencil_skeleton

   # Run Example 7
   cd ../07_resizeable_member
   mpiexec --with-ft mpi -n 4 ./resizeable

Common Patterns Across Examples
--------------------------------

Initialization (Modern)
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <fenix.hpp>

   MPI_Init(&argc, &argv);
   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 3});

Data Recovery (Modern)
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   using namespace fenix::data;

   // Create and store
   group_create(group_id);
   member_create(group_id, member_id, &data, size, MPI_DOUBLE);
   member_store(group_id, SUBSET_FULL);  // SUBSET_FULL = checkpoint all elements
   commit_barrier(group_id);

   // Restore
   member_restore(group_id, member_id);

Inline Recovery with Callback
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   fenix::callback_register([&](MPI_Comm repaired, int err) {
     data::group_create(group_id);
     data::member_restore(group_id, member_id, NULL, 0);
     // Continue execution from here
   });

Next Steps
----------

After exploring the examples:

📚 **Learn More:**

- :doc:`/tutorials/index` - Guided step-by-step tutorials
- :doc:`/guides/index` - Deep conceptual explanations
- :doc:`/api/index` - Complete API reference

🔨 **Build Something:**

- :doc:`/migration-checklist` - Convert your MPI app to use Fenix
- :doc:`/howto/debug-fenix-app` - Debug Fenix applications
- Use Example 8 as a template for your code

Getting Help
------------

- 📖 See :doc:`/troubleshooting` for common issues
- 💡 Check :doc:`/faq` for frequently asked questions
- 🔍 Explore :doc:`/howto/index` for specific tasks
