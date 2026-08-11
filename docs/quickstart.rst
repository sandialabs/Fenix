Quick Start Guide
=================

This guide will get you up and running with Fenix in under 10 minutes. By the end, you'll have built and run your first fault-tolerant MPI program.

Prerequisites
-------------

Before starting, ensure you have:

* **Open MPI 5.0 or later** with ULFM (User Level Failure Mitigation) support
* A C/C++ compiler (gcc, clang, or similar)
* CMake 3.12 or later
* Basic familiarity with MPI programming

.. tip::
   If you don't have Open MPI 5+ with ULFM support, see :doc:`installation` for detailed build instructions.

Step 1: Build and Install Fenix (5 minutes)
--------------------------------------------

Clone the repository and build:

.. code-block:: bash

   # Clone Fenix
   git clone https://github.com/sandialabs/Fenix.git
   cd Fenix

   # Create build directory
   mkdir build && cd build

   # Configure with your MPI compiler
   cmake ../ \
     -DCMAKE_C_COMPILER=mpicc \
     -DCMAKE_CXX_COMPILER=mpicxx \
     -DCMAKE_INSTALL_PREFIX=$HOME/fenix \
     -DBUILD_EXAMPLES=ON

   # Build and install
   make -j4
   make install

.. note::
   If you encounter "multiple MPI versions" segfaults, add ``-DFENIX_SYSTEM_INC_FIX=ON`` to the cmake command.

Verify the installation:

.. code-block:: bash

   # Check that Fenix was installed
   ls $HOME/fenix/lib/libfenix.a
   ls $HOME/fenix/include/fenix.h

Step 2: Your First Fault-Tolerant Program (2 minutes)
------------------------------------------------------

Let's start with a simple "Hello World" program using the **modern C++ API**.

Create a file called ``hello_fenix.cpp``:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>
   #include <stdio.h>

   int main(int argc, char** argv) {
     // Initialize MPI
     MPI_Init(&argc, &argv);

     // Initialize Fenix with modern API - creates resilient communicator
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 1});

     // Check for errors
     if (fenix::error() != FENIX_SUCCESS) {
       printf("Fenix initialization failed\n");
       return 1;
     }

     // Get rank and size from resilient communicator
     int rank, size;
     MPI_Comm_rank(res_comm, &rank);
     MPI_Comm_size(res_comm, &size);

     // Check if we're an initial or recovered rank
     if (fenix::role() == fenix::INITIAL_RANK) {
       printf("Hello from initial rank %d of %d active ranks\n", rank, size);
     } else {
       printf("Hello from recovered rank %d (I replaced a failed rank!)\n", rank);
     }

     // Clean up
     Fenix_Finalize();
     MPI_Finalize();

     return 0;
   }

**Key Features of Modern API:**

1. **Include fenix.hpp** for C++ API (cleaner than C API)
2. **fenix::init()** with designated initializers ``.out_comm`` and ``.spares``
3. **fenix::role()** to check if initial or recovered rank
4. **fenix::error()** for type-safe error checking
5. **Much cleaner** than old multi-parameter ``Fenix_Init()``

.. tip::
   **C API still available!** If you need C, use ``#include <fenix.h>`` and ``Fenix_Init()``.
   But the C++ API is recommended for new code.

Step 3: Compile and Run (2 minutes)
------------------------------------

Compile your program with C++:

.. code-block:: bash

   mpicxx -std=c++17 hello_fenix.cpp -o hello_fenix \
     -I$HOME/fenix/include \
     -L$HOME/fenix/lib -lfenix

.. note::
   C++17 or later is required for the modern API (designated initializers).

Run with fault tolerance enabled:

.. code-block:: bash

   # Run with 4 total ranks (3 active + 1 spare)
   mpiexec --with-ft mpi -n 4 ./hello_fenix

**Expected Output:**

.. code-block:: text

   Hello from initial rank 0 of 3 active ranks
   Hello from initial rank 1 of 3 active ranks
   Hello from initial rank 2 of 3 active ranks

Notice that only 3 ranks are active, even though we launched 4. The 4th rank is held as a spare for recovery.

.. tip::
   Use ``fenix::role()`` to detect if a rank is:

   - ``INITIAL_RANK``: First time through, no failures yet
   - ``RECOVERED_RANK``: Was a spare, now replacing a failed rank
   - ``SURVIVOR_RANK``: Was active before failure, survived and continued

.. important::
   The ``--with-ft mpi`` flag is **required** to enable MPI fault tolerance. Without it, your program may hang or crash when failures occur.

Step 4: See Recovery in Action (1 minute)
------------------------------------------

Now let's see what happens when a rank fails. We'll use the included example that intentionally kills a rank:

.. code-block:: bash

   cd $HOME/Fenix/build/examples/01_hello_world

   # Run the example that kills rank 1
   mpiexec --with-ft mpi -n 4 ./fenix_hello_world 1

**What Happens:**

1. All ranks start normally
2. Rank 1 kills itself (simulating a failure)
3. Fenix detects the failure
4. The spare rank automatically replaces the failed rank
5. Execution continues with recovered communicator
6. All ranks complete successfully

**Sample Output:**

.. code-block:: text

   hello world: hostname, old rank: 0, new rank: 0, active ranks: 3, ranks before: 4
   hello world: hostname, old rank: 2, new rank: 1, active ranks: 2, ranks before: 4
   hello world: hostname, old rank: 3, new rank: 2, active ranks: 2, ranks before: 4
   Rank 0 sees failed processes [1]
   Rank 1 sees failed processes [1]
   Rank 2 sees failed processes [1]

Notice how:

* The spare rank (originally rank 3) became the new rank 2
* All surviving ranks detected the failure
* The program completed successfully despite the failure

Understanding What Just Happened
---------------------------------

Fenix provides **automatic process recovery**:

1. **Spare Ranks**: You designate some ranks as spares during initialization
2. **Failure Detection**: MPI runtime detects when a rank fails
3. **Automatic Repair**: Fenix rebuilds the communicator using spare ranks
4. **Transparent Recovery**: Your application continues with minimal interruption

**Key Concepts:**

* **Fenix Communicator**: A resilient MPI communicator that auto-repairs on failure
* **Spare Ranks**: Reserved ranks that replace failed ranks during recovery
* **longjmp Recovery**: By default, Fenix jumps back to Fenix_Init after recovery using ``longjmp`` (a C library function that jumps to a saved program state). This can be disabled in favor of inline recovery or exception-based recovery.
* **Data Recovery**: Optional feature to checkpoint/restore application data to in-memory redundant storage (covered in tutorials)

Next Steps
----------

Now that you have Fenix working, explore further:

📚 **Learn More:**

* :doc:`tutorials/index` - Step-by-step guided tutorials
* :doc:`guides/process-recovery` - Deep dive into how recovery works
* :doc:`howto/choose-recovery-pattern` - longjmp vs. no-jump patterns

🔨 **Build Something:**

* :doc:`/migration-checklist` - Convert your existing MPI app to use Fenix
* :doc:`guides/data-recovery` - Add data checkpoint/restore
* :doc:`examples/index` - Explore more complex examples

📖 **Reference:**

* :doc:`api/index` - Complete API documentation
* :doc:`troubleshooting` - Common issues and solutions
* :doc:`faq` - Frequently asked questions

Common Quick Start Issues
--------------------------

**"mpiexec: command not found"**
   Ensure Open MPI is in your PATH: ``export PATH=/path/to/openmpi/bin:$PATH``

**"cannot find -lfenix"**
   Add Fenix lib to your library path: ``export LD_LIBRARY_PATH=$HOME/fenix/lib:$LD_LIBRARY_PATH``

**Program hangs at MPI_Init**
   Make sure you're using ``--with-ft mpi`` flag with mpiexec

**Segfault in MPI functions**
   You may have multiple MPI versions. Rebuild Fenix with ``-DFENIX_SYSTEM_INC_FIX=ON``

For more troubleshooting help, see :doc:`troubleshooting`.

Summary
-------

**What You've Learned:**

✅ How to build and install Fenix

✅ The basic structure of a Fenix program

✅ How to compile and run with fault tolerance enabled

✅ How Fenix automatically recovers from rank failures

**What's Different from Regular MPI:**

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Aspect
     - Regular MPI
     - Fenix MPI
   * - Initialization
     - ``MPI_Init``
     - ``MPI_Init`` + ``Fenix_Init``
   * - Communicator
     - ``MPI_COMM_WORLD``
     - Fenix resilient communicator
   * - Rank Failures
     - Program crashes
     - Automatic recovery
   * - Running
     - ``mpiexec -n N``
     - ``mpiexec --with-ft mpi -n N``
   * - Spare Resources
     - All ranks active
     - Some ranks reserved as spares

You're now ready to build fault-tolerant MPI applications with Fenix! 🎉
