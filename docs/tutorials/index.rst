Tutorials
=========

These hands-on tutorials will guide you step-by-step through learning Fenix, from your first fault-tolerant program to advanced recovery patterns.

**Learning Path:** Complete these tutorials in order to build your understanding progressively.

.. toctree::
   :maxdepth: 1
   :caption: Tutorial Series:

   01-first-program
   02-data-recovery
   03-resume-modes
   04-message-logging

Prerequisites
-------------

Before starting the tutorials, ensure you have:

✓ Fenix installed and working (see :doc:`/quickstart`)

✓ Basic MPI knowledge (``MPI_Init``, ``MPI_Send/Recv``, ``MPI_Comm_rank/size``)

✓ C++17 or later compiler (for C++ API tutorials)

✓ Ability to run ``mpiexec --with-ft mpi`` commands

Tutorial Overview
-----------------

Tutorial 1: Your First Fault-Tolerant Program
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Time:** 20 minutes | **Difficulty:** Beginner

Learn the basics of Fenix by building a simple fault-tolerant "Hello World" program. You'll understand:

- How to initialize Fenix with spare ranks
- What happens when a rank fails
- The concept of automatic process recovery
- Basic recovery patterns (longjmp vs inline)

:doc:`Start Tutorial 1 → <01-first-program>`

Tutorial 2: Adding Data Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Time:** 30 minutes | **Difficulty:** Intermediate

Extend your program with data checkpointing and recovery. You'll learn:

- How to create data groups and members
- Checkpointing application state
- Restoring data after failures
- Using data subsets for partial recovery

:doc:`Start Tutorial 2 → <02-data-recovery>`

Tutorial 3: Resume Modes and Recovery Control Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Time:** 20-30 minutes | **Difficulty:** Intermediate

Learn how Fenix communicates failures to your application through resume modes. You'll discover:

- The three resume modes: THROW, RETURN, and JUMP
- When each mode is appropriate for your application
- How callbacks work with any resume mode
- How resume modes interact with message logging

:doc:`Start Tutorial 3 → <03-resume-modes>`

Tutorial 4: Message Logging for Seamless Recovery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Time:** 45 minutes | **Difficulty:** Advanced

Implement message logging for zero-recomputation recovery. You'll explore:

- What message logging is and why it's powerful
- Creating message logs with regions and windows
- Automatic message replay with INLINE_AUTOSYNC
- Complete iterative solver with message logging
- Performance optimization strategies

:doc:`Start Tutorial 4 → <04-message-logging>`

Learning Tips
-------------

**Hands-On Practice**

Type out the code yourself rather than copy-pasting. This helps build muscle memory and understanding.

**Experiment**

Each tutorial includes exercises to try variations. Don't skip these—they reinforce learning.

**Use the Reference**

Keep the :doc:`/api/index` open while working through tutorials to explore function details.

**Ask Questions**

If something isn't clear, check the :doc:`/faq` or :doc:`/troubleshooting` guide.

After the Tutorials
-------------------

Once you complete these tutorials, you'll be ready to:

🔨 Convert existing MPI applications to use Fenix (:doc:`/migration-checklist`)

🏗️ Design fault-tolerant applications from scratch

📚 Explore advanced topics in the :doc:`/guides/index`

🎯 Reference the complete :doc:`/api/index` for production development

Need Help?
----------

- 📖 **Examples:** See :doc:`/examples/index` for complete working programs
- 🔍 **How-To Guides:** Check :doc:`/howto/index` for specific tasks
- 💡 **Concepts:** Read :doc:`/guides/index` for deeper understanding
- 🐛 **Problems:** Visit :doc:`/troubleshooting` for common issues
