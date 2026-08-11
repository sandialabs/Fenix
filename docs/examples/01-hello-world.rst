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

.. code-block:: c
   :caption: mpi/mpi_hello_world.c
   :linenos:

   #include <mpi.h>
   #include <stdio.h>

   int main(int argc, char **argv) {
     MPI_Init(&argc, &argv);

     int world_size;
     MPI_Comm_size(MPI_COMM_WORLD, &world_size);

     int world_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

     char processor_name[MPI_MAX_PROCESSOR_NAME];
     int name_len;
     MPI_Get_processor_name(processor_name, &name_len);

     printf("Hello world from processor %s, rank %d out of %d processors\n",
            processor_name, world_rank, world_size);

     MPI_Finalize();
     return 0;
   }

This is a standard MPI hello world. **If any rank fails during execution, the entire program crashes.**

The Fenix Version
-----------------

Now let's see the fault-tolerant version with Fenix:

.. code-block:: c
   :caption: fenix/fenix_hello_world.c
   :linenos:
   :emphasize-lines: 22-37, 45-48, 62-77

   #include <fenix.h>
   #include <mpi.h>
   #include <stdio.h>
   #include <signal.h>
   #include <stdlib.h>
   #include <sys/types.h>
   #include <unistd.h>
   #include <assert.h>

   const int kKillID = 1;

   int main(int argc, char** argv) {

     if (argc < 2) {
       printf("Usage: %s <# spare ranks> \n", *argv);
       exit(0);
     }

     int old_world_size, new_world_size = -1;
     int old_rank = 1, new_rank = -1;
     int spare_ranks = atoi(argv[1]);

     MPI_Init(&argc, &argv);

     MPI_Barrier(MPI_COMM_WORLD);
     MPI_Comm world_comm;
     MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
     MPI_Comm_size(world_comm, &old_world_size);
     MPI_Comm_rank(world_comm, &old_rank);

     int fenix_status;
     int recovered = 0;
     MPI_Comm new_comm;
     int error;
     Fenix_Init(
       &fenix_status, world_comm, &new_comm, &argc, &argv, spare_ranks, &error
     );

     if (fenix_status != FENIX_ROLE_INITIAL_RANK) {
       MPI_Comm_size(new_comm, &new_world_size);
       MPI_Comm_rank(new_comm, &new_rank);
       recovered = 1;
     }

     if (old_rank == kKillID && recovered == 0) {
       pid_t pid = getpid();
       kill(pid, SIGTERM);
     }

     MPI_Barrier(new_comm);

     char processor_name[MPI_MAX_PROCESSOR_NAME];
     int name_len;
     MPI_Get_processor_name(processor_name, &name_len);

     printf(
       "hello world: %s, old rank (MPI_COMM_WORLD): %d, new rank: %d, active "
       "ranks: %d, ranks before process failure: %d\n",
       processor_name, old_rank, new_rank, new_world_size, old_world_size
     );

     int *fails, num_fails;
     num_fails = Fenix_Process_fail_list(&fails);

     int max = 100, used;
     char fails_str[max];
     used = snprintf(fails_str, max, "Rank %d sees failed processes [", new_rank);
     assert(used > 0 && used < max);
     for (int i = 0; i < num_fails; i++) {
       used = snprintf(
         fails_str, max, "%s%s%d", fails_str, (i == 0 ? "" : ", "), fails[i]
       );
       assert(used > 0 && used < max);
     }
     used = snprintf(fails_str, max, "%s]", fails_str);
     assert(used > 0 && used < max);
     printf("%s\n", fails_str);

     Fenix_Finalize();
     MPI_Finalize();

     return 0;
   }

Code Walkthrough
----------------

Let's break down the key differences:

1. Spare Ranks Setup
^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int spare_ranks = atoi(argv[1]);

The program accepts the number of spare ranks as a command-line argument. Spare ranks are reserved to replace failed ranks during recovery.

**Rule of thumb:** Use 1 spare rank for small jobs, or 5-10% of total ranks for large jobs.

2. Fenix Initialization
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   MPI_Comm world_comm;
   MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);

   int fenix_status, error;
   MPI_Comm new_comm;
   Fenix_Init(
     &fenix_status, world_comm, &new_comm, &argc, &argv, spare_ranks, &error
   );

**Key points:**

- Duplicate ``MPI_COMM_WORLD`` before passing to Fenix
- ``Fenix_Init`` creates a new resilient communicator (``new_comm``)
- ``fenix_status`` tells you what role this rank has after initialization

3. Understanding fenix_status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   if (fenix_status != FENIX_ROLE_INITIAL_RANK) {
     MPI_Comm_size(new_comm, &new_world_size);
     MPI_Comm_rank(new_comm, &new_rank);
     recovered = 1;
   }

The ``fenix_status`` can be:

- ``FENIX_ROLE_INITIAL_RANK``: This is an original active rank (normal case)
- ``FENIX_ROLE_RECOVERED_RANK``: This rank was a spare and is now active (replacing a failed rank)
- ``FENIX_ROLE_SURVIVOR_RANK``: This rank survived a failure and recovered

4. Simulating a Failure
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   if (old_rank == kKillID && recovered == 0) {
     pid_t pid = getpid();
     kill(pid, SIGTERM);
   }

This intentionally kills rank 1 (``kKillID``) to demonstrate recovery. The ``recovered == 0`` check ensures we only kill once, not after recovery.

5. Querying Failed Ranks
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int *fails, num_fails;
   num_fails = Fenix_Process_fail_list(&fails);

   // Print the list of failed ranks
   for (int i = 0; i < num_fails; i++) {
     printf("%d ", fails[i]);
   }

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
