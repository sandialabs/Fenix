Decision Tree: Choosing Recovery Pattern
=========================================

This guide helps you choose the right recovery pattern for your application.

.. _decision-recovery-pattern:

Decision Flowchart
------------------

.. graphviz::
   :caption: Recovery Pattern Decision Tree

   digraph decision_tree {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       start [label="What language\nis your app?", shape=diamond, fillcolor=lightyellow];

       c_lang [label="C", fillcolor=lightblue];
       cpp_lang [label="C++", fillcolor=lightblue];

       c_raii [label="Do you use\nRAII patterns?\n(smart ptrs,\ncontainers,\ndestructors?)", shape=diamond, fillcolor=lightyellow];

       cpp_raii [label="Do you need\nRAII safety?\n(smart ptrs,\ndestructors,\nexceptions?)", shape=diamond, fillcolor=lightyellow];

       simple_c [label="Simple\nrestart?", shape=diamond, fillcolor=lightyellow];
       complex_c [label="Complex\nstate?", shape=diamond, fillcolor=lightyellow];

       longjmp [label="LONGJMP\nEasy setup", fillcolor=lightcoral];
       inline_c [label="INLINE\nMost control", fillcolor=lightgreen];
       exception [label="EXCEPTION\nBest for C++", fillcolor=lightgreen];
       inline_cpp [label="INLINE\nFlexibility", fillcolor=lightgreen];
       longjmp_cpp [label="LONGJMP\nQuick start", fillcolor=yellow];

       start -> c_lang [label="C"];
       start -> cpp_lang [label="C++"];

       c_lang -> c_raii;
       c_raii -> simple_c [label="NO"];
       c_raii -> complex_c [label="YES"];

       simple_c -> longjmp [label="YES"];
       simple_c -> inline_c [label="NO"];
       complex_c -> inline_c [label="YES"];
       complex_c -> longjmp [label="NO"];

       cpp_lang -> cpp_raii;
       cpp_raii -> exception [label="YES"];
       cpp_raii -> inline_cpp [label="NO (prefer)"];
       cpp_raii -> longjmp_cpp [label="NO (legacy)"];
   }

Pattern Comparison
------------------

.. list-table:: Recovery Pattern Comparison
   :header-rows: 1
   :widths: 25 25 25 25

   * - Consideration
     - LONGJMP
     - INLINE
     - EXCEPTION
   * - **Language**
     - C, (C++*)
     - C, C++
     - C++ only
   * - **RAII Safety**
     - ❌ No (UB)
     - ✅ Yes
     - ✅ Yes
   * - **Code Changes**
     - ✅ Minimal
     - ⚠️ Moderate
     - ⚠️ Moderate
   * - **Control Flow**
     - Jump to Init
     - Continue
     - Throw/catch
   * - **State Preserved**
     - ❌ No
     - ✅ Yes
     - ✅ Yes
   * - **Work Lost**
     - ⚠️ All
     - ✅ Minimal
     - ✅ Minimal
   * - **Callbacks Work**
     - ⚠️ Limited
     - ✅ Yes
     - ✅ Yes
   * - **Debugging**
     - ⚠️ Hard
     - ✅ Easy
     - ✅ Easy
   * - **Learning Curve**
     - ✅ Easy
     - ⚠️ Moderate
     - ⚠️ Moderate
   * - **Performance**
     - ⚠️ Poor
     - ✅ Best
     - ✅ Good
   * - **Best for...**
     - Simple C prototypes
     - C apps with complex state
     - Modern C++ apps

.. note::
   **C++ with LONGJMP = Undefined Behavior, NOT recommended!**

Decision Questions
------------------

Question 1: What's your programming language?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**C only:**
  - Consider LONGJMP (simple) or INLINE (more control)
  - Skip EXCEPTION (not available in C)

**C++:**
  - Prefer EXCEPTION (RAII-safe) or INLINE (full control)
  - Avoid LONGJMP (undefined behavior with C++)

Question 2: Do you use RAII patterns?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

RAII patterns include:
  - ``std::unique_ptr``, ``std::shared_ptr``
  - ``std::vector``, ``std::string``, containers
  - ``std::lock_guard``, ``std::unique_lock``
  - ``std::fstream``, file handles
  - Custom classes with destructors
  - Any C++ class with meaningful destructor

**If YES:**
  - Use EXCEPTION or INLINE
  - NEVER use LONGJMP (will leak resources!)

**If NO:**
  - LONGJMP is safe
  - INLINE gives more control

Question 3: How complex is your application state?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Simple state (can restart easily):**
  - Few variables
  - No inter-iteration dependencies
  - Embarrassingly parallel computation
  - Restarting from scratch is cheap

  → LONGJMP works well, minimal code changes

**Complex state (expensive to reinitialize):**
  - Many interdependent variables
  - Long initialization time
  - State built up over many iterations
  - Restarting from scratch is expensive

  → Use INLINE or EXCEPTION with checkpointing

Question 4: Legacy code or new development?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Legacy codebase (large, existing):**
  - Start with LONGJMP (minimal changes)
  - Migrate to INLINE/EXCEPTION incrementally

**New development:**
  - Use EXCEPTION (C++) or INLINE (C)
  - Design for recovery from the start

Specific Use Case Recommendations
----------------------------------

Scientific Simulation (C++)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Long running (days)
- Complex state (grids, particles, etc.)
- Uses modern C++ (vectors, unique_ptrs)
- Frequent checkpoints

.. graphviz::
   :caption: Recommended Pattern for Scientific Simulations

   digraph sim_pattern {
       node [shape=box, style="rounded,filled"];

       recommended [label="✅ RECOMMENDED:\nEXCEPTION", fillcolor=lightgreen];
       alternative [label="⚠️ ALTERNATIVE:\nINLINE", fillcolor=yellow];
       avoid [label="❌ AVOID:\nLONGJMP\n(will leak!)", fillcolor=red, fontcolor=white];

       {rank=same; recommended; alternative; avoid;}
   }

Legacy MPI-based CFD Code (C)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Existing large codebase
- Manual memory management
- Simple iteration loop
- Minimal state

.. graphviz::
   :caption: Recommended Pattern for Legacy C Code

   digraph cfd_pattern {
       node [shape=box, style="rounded,filled"];

       recommended [label="✅ RECOMMENDED:\nLONGJMP\n(easy integration)", fillcolor=lightgreen];
       alternative [label="⚠️ ALTERNATIVE:\nINLINE\n(if state complex)", fillcolor=yellow];

       {rank=same; recommended; alternative;}
   }

Machine Learning Training (C++)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Checkpoint after each epoch
- Large models (expensive initialization)
- Modern C++ with PyTorch/TF C++ API
- Need to continue from checkpoint

.. graphviz::
   :caption: Recommended Pattern for ML Training

   digraph ml_pattern {
       node [shape=box, style="rounded,filled"];

       recommended [label="✅ RECOMMENDED:\nEXCEPTION", fillcolor=lightgreen];
       alternative [label="⚠️ ALTERNATIVE:\nINLINE", fillcolor=yellow];

       {rank=same; recommended; alternative;}
   }

Migration Path
--------------

Step-by-step guide to migrate from LONGJMP to INLINE/EXCEPTION:

.. graphviz::
   :caption: Migration Path

   digraph migration {
       rankdir=TB;
       node [shape=box, style="rounded,filled"];

       step1 [label="Step 1: Current State\nLONGJMP mode\nBasic recovery", fillcolor=lightcoral];

       step2 [label="Step 2: Add Checkpoints\nStill using longjmp\nBut now saving state", fillcolor=lightyellow];

       step3 [label="Step 3: Switch to INLINE (C)\nUse callbacks\nPreserve state", fillcolor=lightgreen];

       step4 [label="Step 4: Switch to EXCEPTION (C++)\nUse exceptions\nFull RAII safety", fillcolor=lightgreen];

       step1 -> step2 [label="Add data recovery"];
       step2 -> step3 [label="Change resume mode"];
       step3 -> step4 [label="Port to C++\n(if applicable)"];
   }

**Step 1 Example (Longjmp):**

.. code-block:: c

   int role;
   Fenix_Init(&role, ...);  // Default: RESUME_JUMP

   if (role == RECOVERED) { /* reinitialize */ }

   for (int i = 0; i < N; i++) {
     // Work
     MPI_Allreduce(...);  // May longjmp back to Init
   }

**Step 2: Add checkpoint infrastructure** (still using longjmp, but now saving state)

**Step 3 Example (Inline):**

.. code-block:: c

   Fenix_set_option(FENIX_RESUME_MODE, FENIX_RESUME_RETURN);
   Fenix_Callback_register(restore_callback, NULL);

   for (int i = state.iter; i < N; i++) {
     state.iter = i;

     int ret = MPI_Allreduce(...);
     if (ret == MPI_ERR_PROC_FAILED) {
       // Recovered! Continue from checkpoint
       continue;
     }

     if (i % 10 == 0) checkpoint();
   }

**Step 4 Example (Exception):**

.. code-block:: cpp

   fenix::init({.resume_mode = fenix::RESUME_THROW});

   try {
     for (int i = state.iter; i < N; i++) {
       state.iter = i;
       MPI_Allreduce(...);  // May throw
       if (i % 10 == 0) checkpoint();
     }
   } catch (fenix::CommException& e) {
     // Recovered! Continue
   }

Quick Reference Guide
---------------------

.. list-table::
   :widths: 50 50

   * - **"I want the simplest possible integration"**
     - → Use LONGJMP (if C or simple C++)
   * - **"I want the best performance and control"**
     - → Use INLINE
   * - **"I want clean C++ code with RAII"**
     - → Use EXCEPTION
   * - **"I have legacy code and want minimal changes"**
     - → Start with LONGJMP, migrate later
   * - **"I cannot afford to lose work"**
     - → Use INLINE or EXCEPTION with checkpointing
   * - **"I'm prototyping"**
     - → Use LONGJMP (fastest to implement)
   * - **"I'm building production system"**
     - → Use EXCEPTION (C++) or INLINE (C)

Symbol Legend
-------------

.. list-table::
   :widths: 20 80

   * - ✅
     - Recommended
   * - ⚠️
     - Acceptable with caveats
   * - ❌
     - Not recommended
   * - UB
     - Undefined Behavior

.. seealso::

   * :doc:`02-longjmp-recovery` - Longjmp details
   * :doc:`03-inline-recovery` - Inline recovery details
   * :doc:`04-exception-recovery` - Exception recovery details
   * :doc:`00-quick-reference` - Quick reference card
