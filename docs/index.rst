Fenix Documentation
===================

Fenix is a software library compatible with the Message Passing Interface (MPI)
that enables **fault recovery without application shutdown**. When MPI ranks fail,
Fenix automatically repairs communicators and recovers application state.

.. important::
   **New to Fenix?** Start with the :doc:`quickstart` to get running in 10 minutes.

Getting Started
===============

.. grid:: 2

   .. grid-item-card:: 🚀 Quick Start
      :link: quickstart
      :link-type: doc

      Get up and running in 10 minutes with a simple fault-tolerant program.

   .. grid-item-card:: 📚 Tutorials
      :link: tutorials/index
      :link-type: doc

      Step-by-step guided learning path from basics to advanced patterns.

   .. grid-item-card:: 📖 Examples
      :link: examples/index
      :link-type: doc

      Working example programs. Start with Example 8 (modern patterns)!

   .. grid-item-card:: 🔍 How-To Guides
      :link: howto/index
      :link-type: doc

      Task-focused guides for specific problems.

Documentation Structure
=======================

This documentation follows the `Diátaxis framework <https://diataxis.fr/>`_:

**Tutorials** (Learning-oriented)
   Step-by-step lessons for learning by doing. Start here if you're new.

**How-To Guides** (Problem-oriented)
   Recipes for solving specific problems. Use when you have a task.

**Explanations** (Understanding-oriented)
   Clarification and discussion of concepts. Read for deeper understanding.

**Reference** (Information-oriented)
   Complete API documentation. Look up specifics here.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started:

   quickstart
   introduction
   installation

.. toctree::
   :maxdepth: 2
   :caption: Learning:

   tutorials/index
   examples/index

.. toctree::
   :maxdepth: 2
   :caption: How-To Guides:

   howto/index

.. toctree::
   :maxdepth: 2
   :caption: Understanding:

   guides/index
   _diagrams

.. toctree::
   :maxdepth: 2
   :caption: Reference:

   api/index
   api-quick-ref
   glossary
   cheat-sheet
   best-practices
   common-mistakes
   migration-checklist

.. toctree::
   :maxdepth: 1
   :caption: Help:

   troubleshooting
   faq

Key Features
============

🛡️ **Process Recovery**
   Automatically repair communicators when ranks fail using spare ranks

💾 **Data Recovery**
   High-performance in-memory checkpoint/restart with RAID-style redundancy

📨 **Message Recovery**
   Optional message logging and replay for seamless fault tolerance

🔄 **Modern C++ API**
   Clean, type-safe interface with exceptions and RAII-safe recovery

⚡ **High Performance**
   Minimal overhead during normal execution, fast recovery times

Quick Example
=============

Modern C++ API with automatic recovery:

.. code-block:: cpp

   #include <fenix.hpp>
   #include <mpi.h>

   int main(int argc, char** argv) {
     MPI_Init(&argc, &argv);

     // Initialize with 3 spare ranks
     MPI_Comm res_comm;
     fenix::init({.out_comm = &res_comm, .spares = 3});

     // Your MPI application code here
     // Uses res_comm instead of MPI_COMM_WORLD
     // Failures are handled automatically!

     Fenix_Finalize();
     MPI_Finalize();
   }

See the :doc:`quickstart` for a complete working example.

What Makes Fenix Different?
============================

Unlike traditional checkpoint/restart that stops and restarts your entire application:

✅ **No full restart** - Continue execution with minimal interruption

✅ **Automatic recovery** - Fenix handles communicator repair transparently

✅ **Flexible patterns** - Choose between three resume modes (JUMP, RETURN, THROW)

✅ **MPI-native** - Works with standard MPI communication patterns

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

