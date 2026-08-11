Tutorial 1: Your First Fault-Tolerant Program
==============================================

**Time:** 20-30 minutes | **Difficulty:** Beginner

Welcome to your first hands-on Fenix tutorial! In this tutorial, you'll build a simple but complete fault-tolerant MPI program from scratch using the modern C++ API. By the end, you'll understand the basic concepts of fault tolerance and have a working program that survives rank failures.

Learning Objectives
-------------------

By completing this tutorial, you will:

* Understand the basic structure of a Fenix program
* Initialize Fenix with spare ranks for automatic recovery
* Distinguish between initial and recovered ranks
* Write recovery logic that handles failures gracefully
* Use exception-based recovery (modern, no longjmp!)
* Test your program with simulated failures

Prerequisites
-------------

Before starting, make sure you have:

✓ Fenix installed (see :doc:`/quickstart` if not)

✓ Basic MPI knowledge (``MPI_Init``, ``MPI_Comm_rank``, ``MPI_Send``/``Recv``)

✓ C++20 compiler (for modern API with designated initializers)

✓ Can run: ``mpiexec --with-ft mpi -n 4 your_program``

.. note::
   **C++ Standard:** This tutorial uses the modern C++ API with designated
   initializers (``{.out_comm = &res_comm}``), which requires C++20.
   Compile with ``-std=c++20`` or use ``-std=c++17`` with compiler extensions.

.. tip::
   If you're not comfortable with MPI yet, review basic MPI tutorials first. This tutorial assumes you know how to write simple MPI programs.

The Problem We're Solving
-------------------------

Imagine you're running a scientific simulation on a supercomputer with thousands of nodes. Without fault tolerance, if **any single node fails**, your entire job crashes and you lose hours of computation. With Fenix, your program can:

1. Detect when a rank fails
2. Automatically replace it with a spare rank
3. Continue running without restarting from scratch

Let's build a simple program to demonstrate this.

Part 1: Basic Structure (5 minutes)
------------------------------------

Create a new file called ``my_first_fenix.cpp``. We'll build it step by step.

Step 1.1: Includes and Main Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start with the basic skeleton:

.. code-block:: cpp

   #include <mpi.h>
   #include <fenix.hpp>
   #include <stdio.h>
   #include <signal.h>

   int main(int argc, char** argv) {
     // Initialize standard MPI
     MPI_Init(&argc, &argv);

     // TODO: Add Fenix initialization

     // TODO: Add application code

     // Clean up
     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

**What's happening:**

* ``#include <fenix.hpp>`` gives us the modern C++ API (cleaner than C API)
* ``<signal.h>`` will be used later to simulate failures
* We still need ``MPI_Init`` - Fenix works *with* MPI, not instead of it

Step 1.2: Initialize Fenix with Spare Ranks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Now add Fenix initialization. Replace the ``TODO: Add Fenix initialization`` comment:

.. code-block:: cpp

   // Initialize Fenix with 1 spare rank for recovery
   MPI_Comm res_comm;
   fenix::init({.out_comm = &res_comm, .spares = 1});

   // Check for errors
   if (fenix::error() != FENIX_SUCCESS) {
     printf("Fenix initialization failed!\n");
     return 1;
   }

**Understanding this code:**

* ``fenix::init()`` uses C++20 designated initializers - much cleaner than the old C API!
* ``.out_comm = &res_comm`` creates a **resilient communicator** (like MPI_COMM_WORLD but survives failures)
* ``.spares = 1`` means "reserve 1 rank as a spare for recovery"
* ``fenix::error()`` checks if initialization succeeded

.. important::
   **Key Concept: Spare Ranks**

   If you launch with ``mpiexec -n 4`` and set ``.spares = 1``, you get:

   * 3 **active ranks** doing work (ranks 0, 1, 2)
   * 1 **spare rank** waiting to replace failures (rank 3)

   The spare doesn't participate in your computation until a failure occurs.

Step 1.3: Get Rank Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add code to get rank and size information:

.. code-block:: cpp

   // Get rank info from the resilient communicator
   int rank, size;
   MPI_Comm_rank(res_comm, &rank);
   MPI_Comm_size(res_comm, &size);

   printf("I am rank %d of %d active ranks\n", rank, size);

**Important:** Always use ``res_comm`` (the Fenix communicator), not ``MPI_COMM_WORLD``, for your application logic!

Step 1.4: Compile and Test
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Let's make sure it works so far:

.. code-block:: bash

   # Compile with C++17 (required for designated initializers)
   mpicxx -std=c++17 my_first_fenix.cpp -o my_first_fenix \
     -I$HOME/fenix/include -L$HOME/fenix/lib -lfenix

   # Run with 4 total ranks (3 active + 1 spare)
   mpiexec --with-ft mpi -n 4 ./my_first_fenix

**Expected output:**

.. code-block:: text

   I am rank 0 of 3 active ranks
   I am rank 1 of 3 active ranks
   I am rank 2 of 3 active ranks

Notice only 3 ranks print! The 4th is held as a spare.

.. tip::
   **Troubleshooting:**

   * **"cannot find -lfenix"**: Check your library path
   * **Hangs at MPI_Init**: Make sure you used ``--with-ft mpi``
   * **"designated initializers" error**: Use ``-std=c++17`` or later

Part 2: Adding Recovery Logic (10 minutes)
-------------------------------------------

Now let's add the ability to detect and recover from failures.

Step 2.1: Understand Rank Roles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After initialization, each rank has a **role** that tells you its recovery state:

.. code-block:: cpp

   // Check our role
   if (fenix::role() == fenix::INITIAL_RANK) {
     printf("Rank %d: I'm an initial rank (just started)\n", rank);
   } else if (fenix::role() == fenix::RECOVERED_RANK) {
     printf("Rank %d: I'm a recovered rank (I replaced a failure!)\n", rank);
   } else if (fenix::role() == fenix::SURVIVOR_RANK) {
     printf("Rank %d: I'm a survivor (I survived a failure)\n", rank);
   }

Add this code after getting rank information.

**Understanding roles:**

* ``INITIAL_RANK``: First time through - no failures yet
* ``RECOVERED_RANK``: I'm a spare that just replaced a failed rank
* ``SURVIVOR_RANK``: I'm an original rank that survived while others failed

Step 2.2: Add Work Loop with Failure Simulation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Let's simulate a real application with iterations. Add this before ``Fenix_Finalize()``:

.. code-block:: cpp

   // Simulate application work with iterations
   for (int iter = 0; iter < 10; iter++) {
     // Simulate work
     printf("Rank %d: iteration %d\n", rank, iter);

     // Synchronize all ranks
     MPI_Barrier(res_comm);

     // Simulate a failure: rank 1 dies at iteration 3
     if (rank == 1 && iter == 3) {
       printf("Rank %d: SIMULATING FAILURE!\n", rank);
       raise(SIGKILL);  // Kill this rank
     }
   }

   printf("Rank %d: All iterations completed successfully!\n", rank);

**What this does:**

* Runs 10 iterations of "work" (just printing)
* At iteration 3, rank 1 kills itself
* Other ranks will detect this during the ``MPI_Barrier``

Step 2.3: Run and Observe the Failure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Recompile and run:

.. code-block:: bash

   mpicxx -std=c++17 my_first_fenix.cpp -o my_first_fenix \
     -I$HOME/fenix/include -L$HOME/fenix/lib -lfenix

   mpiexec --with-ft mpi -n 4 ./my_first_fenix

**What happens:**

.. code-block:: text

   Rank 0: iteration 0
   Rank 1: iteration 0
   Rank 2: iteration 0
   ...
   Rank 1: iteration 3
   Rank 1: SIMULATING FAILURE!
   [Process killed]
   Rank 0: iteration 4
   ... [HANGS or CRASHES]

**Problem:** The program doesn't recover! It hangs or crashes because we haven't told Fenix **how** to recover.

Step 2.4: Enable Exception-Based Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add this **immediately after** ``fenix::init()``:

.. code-block:: cpp

   // Configure Fenix to throw exceptions on failure (modern approach)
   fenix::set_option(fenix::RECOVERY_MODE, fenix::REPAIR);
   fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

**What this does:**

* ``RECOVERY_MODE = REPAIR``: Automatically repair the communicator when failures occur
* ``RESUME_MODE = THROW``: Instead of longjmp, throw a C++ exception

.. note::
   **Why exceptions instead of longjmp?**

   The old approach used ``longjmp`` to jump back to ``Fenix_Init``. This has undefined behavior in C++ and can cause bugs. The modern approach uses exceptions, which are well-defined and easier to reason about.

Step 2.5: Wrap Work Loop in Try-Catch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Now wrap your work loop to catch recovery exceptions:

.. code-block:: cpp

   // Main application loop with exception-based recovery
   bool keep_running = true;
   while (keep_running) {
     try {
       // Your work loop goes here
       for (int iter = 0; iter < 10; iter++) {
         printf("Rank %d: iteration %d\n", rank, iter);
         MPI_Barrier(res_comm);

         if (rank == 1 && iter == 3) {
           printf("Rank %d: SIMULATING FAILURE!\n", rank);
           raise(SIGKILL);
         }
       }

       printf("Rank %d: All iterations completed!\n", rank);
       keep_running = false;  // Exit loop on success

     } catch (fenix::CommException& e) {
       // A failure occurred and was recovered!
       printf("Rank %d: Caught failure, recovered communicator!\n", rank);

       // Update our rank (it may have changed after recovery)
       MPI_Comm_rank(e.repaired_comm, &rank);
       MPI_Comm_size(e.repaired_comm, &size);
       res_comm = e.repaired_comm;

       printf("Rank %d: Continuing after recovery...\n", rank);
       // Loop continues with new communicator
     }
   }

**Understanding this pattern:**

1. Try to do work normally
2. If a failure occurs during an MPI call, Fenix throws ``CommException``
3. The exception contains a ``repaired_comm`` - a new, working communicator
4. Update your rank information (it may have changed!)
5. Continue execution with the repaired communicator

Step 2.6: Test Recovery
~~~~~~~~~~~~~~~~~~~~~~~~

Recompile and run again:

.. code-block:: bash

   mpicxx -std=c++17 my_first_fenix.cpp -o my_first_fenix \
     -I$HOME/fenix/include -L$HOME/fenix/lib -lfenix

   mpiexec --with-ft mpi -n 4 ./my_first_fenix

**Expected output:**

.. code-block:: text

   Rank 0: iteration 0
   Rank 1: iteration 0
   Rank 2: iteration 0
   ...
   Rank 1: iteration 3
   Rank 1: SIMULATING FAILURE!
   Rank 0: Caught failure, recovered communicator!
   Rank 2: Caught failure, recovered communicator!
   Rank 0: Continuing after recovery...
   Rank 2: Continuing after recovery...
   Rank 0: iteration 0
   Rank 2: iteration 0
   ...
   Rank 0: All iterations completed!
   Rank 2: All iterations completed!

**Success!** The program recovered and completed.

.. important::
   **What Just Happened:**

   1. Rank 1 died at iteration 3
   2. Ranks 0 and 2 detected the failure at ``MPI_Barrier``
   3. Fenix threw ``CommException`` to all surviving ranks
   4. The spare rank replaced rank 1
   5. The catch block got a repaired communicator
   6. Execution restarted from iteration 0 (we'll improve this next)

Part 3: Improving Recovery (5 minutes)
---------------------------------------

The program works, but restarting from iteration 0 after every failure is inefficient. Let's save progress.

Step 3.1: Add State Tracking
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add a struct to track application state before ``main()``:

.. code-block:: cpp

   struct AppState {
     int rank = -1;
     int iteration = 0;
     bool initialized = false;
   };

Step 3.2: Preserve State Across Iterations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Modify your code to track iteration progress:

.. code-block:: cpp

   AppState state;
   state.rank = rank;
   state.iteration = 0;
   state.initialized = (fenix::role() == fenix::INITIAL_RANK);

   bool keep_running = true;
   while (keep_running) {
     try {
       // Resume from where we left off, not from 0!
       for (int iter = state.iteration; iter < 10; iter++) {
         state.iteration = iter;
         printf("Rank %d: iteration %d\n", rank, iter);

         MPI_Barrier(res_comm);

         if (rank == 1 && iter == 3) {
           printf("Rank %d: SIMULATING FAILURE!\n", rank);
           raise(SIGKILL);
         }
       }

       printf("Rank %d: All iterations completed!\n", rank);
       keep_running = false;

     } catch (fenix::CommException& e) {
       printf("Rank %d: Caught failure at iteration %d\n", rank, state.iteration);

       // Update communicator and rank
       res_comm = e.repaired_comm;
       MPI_Comm_rank(res_comm, &rank);
       state.rank = rank;

       printf("Rank %d: Restarting from iteration %d\n", rank, state.iteration);
       // Continue loop, but from current iteration
     }
   }

**Now when a failure occurs**, survivors continue from their current iteration instead of restarting from 0!

.. note::
   This still has a problem: the **recovered rank** (the new rank 1) doesn't have ``state.iteration`` set correctly. We'll fix this in :doc:`02-data-recovery` using data recovery.

Complete Program
----------------

Here's the complete program you've built:

.. code-block:: cpp

   #include <mpi.h>
   #include <fenix.hpp>
   #include <stdio.h>
   #include <signal.h>

   struct AppState {
     int rank = -1;
     int iteration = 0;
   };

   int main(int argc, char** argv) {
     // Initialize MPI
     MPI_Init(&argc, &argv);

     // Initialize Fenix with 1 spare rank
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     if (fenix::error() != FENIX_SUCCESS) {
       printf("Fenix initialization failed!\n");
       return 1;
     }

     // Configure exception-based recovery (modern approach)
     fenix::set_option(fenix::RECOVERY_MODE, fenix::REPAIR);
     fenix::set_option(fenix::RESUME_MODE, fenix::THROW);

     // Get rank information
     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     printf("Rank %d of %d: role = %s\n", rank, size,
            fenix::role() == fenix::INITIAL_RANK ? "INITIAL" : "RECOVERED");

     // Track application state
     AppState state;
     state.rank = rank;
     state.iteration = 0;

     // Main application loop with recovery
     bool keep_running = true;
     while (keep_running) {
       try {
         // Application work
         for (int iter = state.iteration; iter < 10; iter++) {
           state.iteration = iter;
           printf("Rank %d: iteration %d\n", rank, iter);

           MPI_Barrier(res_comm);

           // Simulate failure
           if (rank == 1 && iter == 3) {
             printf("Rank %d: SIMULATING FAILURE!\n", rank);
             raise(SIGKILL);
           }
         }

         printf("Rank %d: All iterations completed!\n", rank);
         keep_running = false;

       } catch (fenix::CommException& e) {
         printf("Rank %d: Caught failure, recovering...\n", rank);

         // Get repaired communicator
         res_comm = e.repaired_comm;
         MPI_Comm_rank(res_comm, &rank);
         state.rank = rank;

         printf("Rank %d: Continuing from iteration %d\n", rank, state.iteration);
       }
     }

     // Clean up
     Fenix_Finalize();
     MPI_Finalize();
     return 0;
   }

Understanding What You've Built
--------------------------------

Congratulations! You've built a fault-tolerant MPI program. Let's review the key concepts:

**1. Spare Ranks**

You reserve some ranks at initialization to replace failed ranks:

* ``fenix::init({.spares = 1})`` reserves 1 spare
* If you launch with ``-n 4`` and set 1 spare, you get 3 active + 1 spare

**2. Resilient Communicator**

The communicator returned by ``fenix::init()`` automatically repairs itself:

* Use it instead of ``MPI_COMM_WORLD``
* When a rank fails, Fenix repairs it automatically
* The spare rank joins with the same rank ID as the failed rank

**3. Exception-Based Recovery**

Modern C++ API uses exceptions instead of longjmp:

* ``fenix::RESUME_MODE = fenix::THROW`` enables exceptions
* Catch ``fenix::CommException`` to handle failures
* The exception provides the repaired communicator

**4. Recovery Flow**

When a rank fails:

1. MPI detects the failure during an MPI operation
2. Fenix automatically repairs the communicator using a spare
3. Fenix throws ``CommException`` to all surviving ranks
4. Your catch block receives the repaired communicator
5. You update your rank/size and continue execution

**Limitations**

Your current program has some limitations:

* **No data recovery**: Recovered ranks don't have application state
* **Restarts from same iteration**: Not efficient for long computations
* **Simple failure model**: Only handles one failure

These will be addressed in :doc:`02-data-recovery`!

Exercises
---------

Try these exercises to reinforce your learning:

Exercise 1: Multiple Spares
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Modify the program to use 2 spare ranks:

1. Change ``.spares = 1`` to ``.spares = 2``
2. Add a second failure: rank 0 dies at iteration 7
3. Run with ``-n 6`` (4 active + 2 spares)
4. Observe how both failures are recovered

Exercise 2: Different Recovery Strategy
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Try different failure injection patterns:

1. What happens if rank 1 dies at iteration 0?
2. What if two ranks die at the same iteration?
3. What if rank 1 dies, recovers, then dies again?

Exercise 3: Add Useful Work
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Replace the simple print statements with actual MPI communication:

1. Have each rank send its iteration count to rank 0
2. Rank 0 prints the sum of all iteration counts
3. Observe how this behaves during failure/recovery

Exercise 4: Error Handling
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add more robust error handling:

1. Check the return value of all MPI calls
2. Print information about failed ranks using ``fenix::fail_list()``
3. Count how many failures occurred during execution

**Solution hints for Exercise 4:**

.. code-block:: cpp

   catch (fenix::CommException& e) {
     auto failed = fenix::fail_list();
     printf("Rank %d: %zu ranks failed: ", rank, failed.size());
     for (int f : failed) printf("%d ", f);
     printf("\n");

     // ... rest of recovery logic
   }

Common Issues and Tips
----------------------

**Issue: "undefined reference to fenix::init"**

Solution: Make sure you're linking ``-lfenix`` and using C++17 or later.

**Issue: Program hangs at MPI_Barrier after failure**

Solution: Ensure you're using the repaired communicator from the exception, not the old one.

**Issue: Rank numbers change after recovery**

This is expected when you run out of spares! Fenix will shrink the communicator and renumber ranks.

**Issue: Caught exception but iteration restarts from 0**

Make sure you're using ``iter = state.iteration`` in the for loop, not ``iter = 0``.

**Tip: Debugging**

Add verbose output to understand the recovery flow:

.. code-block:: cpp

   printf("Rank %d: Before MPI_Barrier at iter %d\n", rank, iter);
   MPI_Barrier(res_comm);
   printf("Rank %d: After MPI_Barrier at iter %d\n", rank, iter);

Next Steps
----------

You now understand the basics of fault tolerance with Fenix! Here's what to explore next:

**Tutorial 2: Adding Data Recovery** (:doc:`02-data-recovery`)

Learn how to checkpoint application state so recovered ranks can restore data and continue seamlessly.

**Tutorial 3: Inline Recovery Patterns** (:doc:`03-inline-recovery`)

Discover more advanced recovery patterns including callbacks and fine-grained control.

**Concepts: Process Recovery** (:doc:`/guides/process-recovery`)

Deep dive into how Fenix detects failures and repairs communicators.

**API Reference** (:doc:`/api/index`)

Explore the complete Fenix C++ API documentation.

Summary
-------

In this tutorial, you learned:

✅ How to initialize Fenix with spare ranks

✅ The difference between initial, recovered, and survivor ranks

✅ How to use exception-based recovery (modern, no longjmp)

✅ How to catch ``CommException`` and get repaired communicators

✅ Basic state tracking to avoid restarting from scratch

✅ How to simulate and recover from rank failures

**Key Takeaway:** Fenix makes fault tolerance simple by automatically detecting failures, repairing communicators with spare ranks, and providing clean exception-based recovery patterns.

You're now ready to build more sophisticated fault-tolerant applications!
