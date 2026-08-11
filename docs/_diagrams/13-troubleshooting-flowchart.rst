Troubleshooting Flowchart
==========================

Problem diagnosis and resolution guide for common Fenix issues.

.. _troubleshooting-flowchart:

Application Crashes on Failure
-------------------------------

.. graphviz::
   :caption: Crash Troubleshooting

   digraph crash_troubleshooting {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       problem [label="App crashes when\na rank fails", fillcolor=red, fontcolor=white];

       init_check [label="Does Fenix_Init\nreturn success?", shape=diamond, fillcolor=lightyellow];

       init_problem [label="Initialization Problem:\n• MPI built with FT?\n• --with-ft mpi flag?\n• ULFM enabled?\n• Enough ranks?", fillcolor=orange];

       runtime_problem [label="Runtime Problem:\n• Check return codes\n• Check error handlers\n• Check RESUME_MODE\n• Check callbacks", fillcolor=orange];

       problem -> init_check;
       init_check -> init_problem [label="NO"];
       init_check -> runtime_problem [label="YES"];
   }

Segfault After Recovery
------------------------

.. graphviz::
   :caption: Segfault Diagnosis

   digraph segfault {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       segfault [label="Segfault after\nrank recovery", fillcolor=red, fontcolor=white];

       mode [label="Which recovery\nmode?", shape=diamond, fillcolor=lightyellow];

       longjmp_issue [label="LONGJMP:\nVariables corrupted\n→ Use volatile", fillcolor=orange];

       inline_issue [label="INLINE:\nPointers invalid\n→ Recreate pointers", fillcolor=orange];

       exception_issue [label="EXCEPTION:\nRAII violated\n→ Fix exception safety", fillcolor=orange];

       segfault -> mode;
       mode -> longjmp_issue [label="LONGJMP"];
       mode -> inline_issue [label="INLINE"];
       mode -> exception_issue [label="EXCEPTION"];
   }

Data Not Restored Correctly
----------------------------

.. graphviz::
   :caption: Data Recovery Issues

   digraph data_issues {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       problem [label="Recovered rank\nhas wrong data", fillcolor=red, fontcolor=white];

       restore_check [label="Did you call\nmember_restore?", shape=diamond, fillcolor=lightyellow];

       no_restore [label="Must call\nmember_restore!", fillcolor=red, fontcolor=white];

       commit_check [label="Was checkpoint\ncommitted?", shape=diamond, fillcolor=lightyellow];

       no_commit [label="Must call\ncommit()\nafter store()!", fillcolor=red, fontcolor=white];

       details [label="Check:\n• Subset\n• Member ID\n• Group ID", fillcolor=orange];

       problem -> restore_check;
       restore_check -> no_restore [label="NO"];
       restore_check -> commit_check [label="YES"];
       commit_check -> no_commit [label="NO"];
       commit_check -> details [label="YES"];
   }

Deadlock During Recovery
-------------------------

.. graphviz::
   :caption: Deadlock Diagnosis

   digraph deadlock {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       problem [label="Application hangs\nduring recovery", fillcolor=red, fontcolor=white];

       location [label="Where does\nit hang?", shape=diamond, fillcolor=lightyellow];

       fenix_hang [label="In Fenix:\nComm recovery issue\n• Multiple derived comms?\n• Mismatched collectives?", fillcolor=orange];

       callback_hang [label="In callback:\nBlocking MPI call\n• Remove MPI from callback", fillcolor=orange];

       app_hang [label="In app:\nMismatch collective\n• All ranks must participate", fillcolor=orange];

       problem -> location;
       location -> fenix_hang [label="In Fenix"];
       location -> callback_hang [label="In callback"];
       location -> app_hang [label="In app"];
   }

Common Errors and Solutions
----------------------------

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Error
     - Solution
   * - "MPI_ERR_COMM: invalid communicator"
     - Use ``res_comm`` from Fenix_Init
   * - "Fenix_Init returned error -1"
     - Launch with N + S ranks (``mpiexec -n N+S``)
   * - "member_restore: group not found"
     - Call ``group_create`` after Fenix_Init
   * - "Deadlock in MPIX_Comm_agree"
     - Use only ``res_comm``, avoid derived comms
   * - "Variables have garbage values"
     - Use ``volatile`` or switch to inline/exception
   * - "Memory leak after recovery"
     - Switch to exception mode for C++

Debugging Checklist
-------------------

**Compilation & Linking:**

☐ Headers found (``fenix.h``, ``fenix.hpp``)

☐ Library linked (``-lfenix``)

☐ C++20 or later

☐ Correct MPI compiler (``mpicc``/``mpicxx``)

**MPI Setup:**

☐ MPI has fault tolerance (Open MPI 5+)

☐ ``--with-ft mpi`` flag used

☐ Enough ranks (N active + S spares)

☐ ULFM enabled in MPI

**Fenix Initialization:**

☐ ``Fenix_Init`` returns success

☐ Spares parameter > 0

☐ Error code checked

☐ ``res_comm`` used for MPI ops

**Data Recovery (if used):**

☐ ``group_create`` called after Init

☐ ``member_create`` for initial ranks

☐ ``member_define`` for recovered ranks

☐ ``member_store`` called

☐ ``commit`` called (not just store!)

☐ ``member_restore`` called for recovered

**Recovery Mode:**

☐ RESUME_MODE set correctly

☐ Callback registered (if inline/exception)

☐ ``volatile`` used (if longjmp)

☐ Exception caught (if exception mode)

**Runtime:**

☐ Using repaired communicator (``res_comm``)

☐ Not using old/revoked communicator

☐ MPI datatypes recreated if needed

☐ MPI windows recreated if needed

Performance Issues
------------------

.. graphviz::
   :caption: Performance Problem Diagnosis

   digraph performance {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       slow [label="Recovery is\ntoo slow", fillcolor=red, fontcolor=white];

       what [label="What is slow?", shape=diamond, fillcolor=lightyellow];

       checkpoint [label="Checkpoint:\nToo frequent\n→ Reduce frequency", fillcolor=orange];

       recovery [label="Recovery:\nData size\n→ Use subsets", fillcolor=orange];

       overall [label="Overall:\nFrequency too high\n→ Adjust interval", fillcolor=orange];

       slow -> what;
       what -> checkpoint [label="Checkpoint"];
       what -> recovery [label="Recovery"];
       what -> overall [label="Overall"];
   }

**Solutions:**

- Reduce checkpoint frequency
- Use data subsets (partial checkpoints)
- Compress data (if applicable)
- Use async checkpoint (future feature)

.. seealso::

   * :doc:`00-quick-reference` - Common patterns and errors
   * :doc:`12-decision-recovery-pattern` - Choosing the right pattern
   * :doc:`10-checkpoint-timeline` - Performance tuning
