Example 1: Hello World with Fault Tolerance
============================================

.. contents:: In This Example
   :local:
   :depth: 2

Overview
--------

This example demonstrates the minimal code needed to create a fault-tolerant MPI program with Fenix. It shows:

- Basic Fenix initialization and finalization
- Creating a resilient communicator with spare ranks
- Detecting and recovering from rank failures
- Querying which ranks failed

**What You'll Learn:**

✓ The basic structure of a Fenix program

✓ How spare ranks enable recovery

✓ What happens when a rank fails

✓ How to check which ranks failed

**Time to Complete:** 10 minutes

**Difficulty:** Beginner

Location
--------

- **Source:** ``examples/01_hello_world/``
- **Plain MPI version:** ``mpi/mpi_hello_world.c``
- **Fenix version:** ``fenix/fenix_hello_world.c``

Prerequisites
-------------

- Basic understanding of MPI (``MPI_Init``, ``MPI_Comm_rank``, ``MPI_Comm_size``)
- Fenix installed and working (see :doc:`/quickstart`)

The Plain MPI Version
----------------------

First, let's look at the plain MPI program without fault tolerance:

.. literalinclude:: ../../examples/01_hello_world/mpi/mpi_hello_world.c
   :language: c
   :caption: mpi/mpi_hello_world.c
   :linenos:
   :start-after: // [mpi-hello-world-full]
   :end-before: // [mpi-hello-world-full]

This is a standard MPI hello world. **If any rank fails during execution, the entire program crashes.**

The Fenix Version
-----------------

Now let's see the fault-tolerant version with Fenix:

.. literalinclude:: ../../examples/01_hello_world/fenix/fenix_hello_world.c
   :language: c
   :caption: fenix/fenix_hello_world.c
   :linenos:
   :start-after: // [fenix-hello-world-full]
   :end-before: // [fenix-hello-world-full]
   :emphasize-lines: 21-40, 43-47, 61-76

Code Walkthrough
----------------

Let's break down the key differences:

1. Spare Ranks Setup
^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/01_hello_world/fenix/fenix_hello_world.c
   :language: c
   :start-after: // [spare-ranks-setup]
   :end-before: // [spare-ranks-setup]

The program accepts the number of spare ranks as a command-line argument. Spare ranks are reserved to replace failed ranks during recovery.

**Rule of thumb:** Use 1 spare rank for small jobs, or 5-10% of total ranks for large jobs.

2. Fenix Initialization
^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/01_hello_world/fenix/fenix_hello_world.c
   :language: c
   :start-after: // [fenix-init]
   :end-before: // [fenix-init]

**Key points:**

- Duplicate ``MPI_COMM_WORLD`` before passing to Fenix
- ``Fenix_Init`` creates a new resilient communicator (``new_comm``)
- ``fenix_status`` tells you what role this rank has after initialization

3. Understanding fenix_status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/01_hello_world/fenix/fenix_hello_world.c
   :language: c
   :start-after: // [check-status]
   :end-before: // [check-status]

The ``fenix_status`` can be:

- ``FENIX_ROLE_INITIAL_RANK``: This is an original active rank (normal case)
- ``FENIX_ROLE_RECOVERED_RANK``: This rank was a spare and is now active (replacing a failed rank)
- ``FENIX_ROLE_SURVIVOR_RANK``: This rank survived a failure and recovered

4. Simulating a Failure
^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/01_hello_world/fenix/fenix_hello_world.c
   :language: c
   :start-after: // [simulate-failure]
   :end-before: // [simulate-failure]

This intentionally kills rank 1 (``kKillID``) to demonstrate recovery. The ``recovered == 0`` check ensures we only kill once, not after recovery.

5. Querying Failed Ranks
^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../examples/01_hello_world/fenix/fenix_hello_world.c
   :language: c
   :start-after: // [query-failures]
   :end-before: // [query-failures]

``Fenix_Process_fail_list`` returns an array of rank IDs that have failed. This is useful for:

- Logging which ranks failed
- Determining if you need to regenerate lost data
- Debugging failure patterns

Building the Example
--------------------

From the Fenix build directory:

.. code-block:: bash

   cd examples/01_hello_world
   make

Or manually:

.. code-block:: bash

   mpicc -I/path/to/fenix/include fenix_hello_world.c \
     -L/path/to/fenix/lib -lfenix -o fenix_hello_world

Running the Example
-------------------

Basic Run (No Failures)
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Run with 4 total ranks, 1 spare (3 active ranks)
   mpiexec --with-ft mpi -n 4 ./fenix_hello_world 1

**Expected Output:**

.. code-block:: text

   hello world: hostname, old rank: 0, new rank: 0, active ranks: 3, ranks before: 4
   hello world: hostname, old rank: 1, new rank: 1, active ranks: 3, ranks before: 4
   hello world: hostname, old rank: 2, new rank: 2, active ranks: 3, ranks before: 4
   Rank 0 sees failed processes []
   Rank 1 sees failed processes []
   Rank 2 sees failed processes []

Notice:

- 3 ranks are active (0, 1, 2)
- 1 rank is held in reserve as spare (rank 3)
- No failures occurred (empty fail list)

Run with Simulated Failure
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The example kills rank 1 to demonstrate recovery:

.. code-block:: bash

   mpiexec --with-ft mpi -n 4 ./fenix_hello_world 1

**What Happens:**

1. All 4 ranks start (3 active + 1 spare)
2. Rank 1 kills itself with ``SIGTERM``
3. Fenix detects the failure
4. Spare rank 3 replaces failed rank 1
5. Communicator is repaired
6. By default, Fenix uses ``longjmp`` to jump back to ``Fenix_Init``
7. Execution continues with recovered ranks

**Expected Output:**

.. code-block:: text

   hello world: hostname, old rank: 0, new rank: 0, active ranks: 2, ranks before: 4
   hello world: hostname, old rank: 2, new rank: 1, active ranks: 2, ranks before: 4
   Rank 0 sees failed processes [1]
   Rank 1 sees failed processes [1]

Notice:

- Only 2 ranks remain active (original rank 1 failed, we used the spare)
- Original rank 2 became new rank 1 (spare replaced failed rank)
- Both ranks detected rank 1 failed

Understanding the Recovery Process
-----------------------------------

Here's what happens step-by-step when rank 1 fails:

.. code-block:: text

   Time  | Rank 0        | Rank 1 (dies) | Rank 2        | Rank 3 (spare)
   ------+---------------+---------------+---------------+----------------
   T0    | Running       | Running       | Running       | Waiting
   T1    | Running       | KILLED        | Running       | Waiting
   T2    | Detects fail  | [dead]        | Detects fail  | Activated
   T3    | longjmp back  | [dead]        | longjmp back  | Replaces rank 1
   T4    | Continue      | [dead]        | Continue      | Continue as new rank

**Key Insight:** By default, Fenix uses ``longjmp`` to return execution to ``Fenix_Init`` after recovery. This emulates traditional checkpoint/restart but without restarting the entire application.

Exercises
---------

Try modifying the example to experiment:

1. **More Spares:**

   .. code-block:: bash

      # Run with 2 spare ranks
      mpiexec --with-ft mpi -n 5 ./fenix_hello_world 2

2. **Kill Different Rank:**

   Change ``kKillID`` to a different value and rebuild.

3. **Multiple Failures:**

   Modify to kill multiple ranks (be sure you have enough spares!)

4. **No Spares:**

   What happens if you run with 0 spares? Try it:

   .. code-block:: bash

      mpiexec --with-ft mpi -n 3 ./fenix_hello_world 0

   (Answer: Program will abort since there's no spare to replace failed rank)

Common Questions
----------------

**Q: Why do I need spare ranks?**

A: Spare ranks are the pool of resources used to replace failed ranks. Without spare ranks, recovery isn't possible.

**Q: Are spare ranks doing nothing?**

A: Yes, they wait idle until needed. This is the tradeoff for fault tolerance—you reserve some resources for recovery.

**Q: What if I run out of spare ranks?**

A: If more ranks fail than you have spare ranks, recovery will fail and the program will abort. Choose your spare count based on expected failure rates.

**Q: Can I use spare ranks for computation?**

A: No, spare ranks must remain idle to be available for recovery. If you want to use all ranks, consider alternative approaches like data replication without spares.

**Q: What is longjmp recovery?**

A: After recovering, Fenix uses ``longjmp`` to return execution to the point right after ``Fenix_Init``. This is like a checkpoint restart but without restarting the whole program. You can disable this behavior for more control (see :doc:`/howto/choose-recovery-pattern`).

Next Steps
----------

Now that you understand basic process recovery:

📚 **Continue Learning:**

- :doc:`/tutorials/01-first-program` - Guided tutorial building from scratch
- :doc:`/guides/process-recovery` - Deep dive into recovery mechanics

🔨 **Try More Features:**

- :doc:`/guides/data-recovery` - Checkpoint and restore application state
- :doc:`/howto/choose-recovery-pattern` - Learn about no-jump recovery pattern

📖 **API Reference:**

- :c:func:`Fenix_Init` - Complete initialization documentation
- :c:func:`Fenix_Process_fail_list` - Query failed ranks
- :c:func:`Fenix_Finalize` - Cleanup

Summary
-------

**You've learned:**

✓ How to initialize Fenix with spare ranks

✓ What happens when a rank fails and recovers

✓ How to query which ranks have failed

✓ The role of spare ranks in recovery

**Key Takeaways:**

- Fenix requires spare ranks for recovery
- Recovery is automatic after failure detection
- Default recovery uses longjmp to restart from Fenix_Init
- All surviving ranks can query the list of failed ranks
