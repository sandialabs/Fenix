Visual Aids and Diagrams
=========================

Fenix provides visual diagrams to help understand complex concepts. These diagrams are generated using Graphviz and show recovery flows, architecture, and decision trees.

.. toctree::
   :maxdepth: 1
   :caption: Available Diagrams

   _diagrams/00-quick-reference
   _diagrams/01-basic-recovery-flow
   _diagrams/02-longjmp-recovery
   _diagrams/03-inline-recovery
   _diagrams/04-exception-recovery
   _diagrams/05-architecture
   _diagrams/06-data-group-structure
   _diagrams/07-rank-roles
   _diagrams/08-message-log-structure
   _diagrams/09-spare-rank-layout
   _diagrams/10-checkpoint-timeline
   _diagrams/12-decision-recovery-pattern
   _diagrams/13-troubleshooting-flowchart

Quick Reference
---------------

See :doc:`_diagrams/00-quick-reference` for a quick overview of Fenix concepts.

Recovery Flows
--------------

- :doc:`_diagrams/01-basic-recovery-flow` - Complete failure detection and recovery process
- :doc:`_diagrams/02-longjmp-recovery` - Recovery using longjmp
- :doc:`_diagrams/03-inline-recovery` - Inline recovery without longjmp
- :doc:`_diagrams/04-exception-recovery` - C++ exception-based recovery

Architecture
------------

- :doc:`_diagrams/05-architecture` - System architecture overview
- :doc:`_diagrams/06-data-group-structure` - Data group organization
- :doc:`_diagrams/07-rank-roles` - Rank roles and responsibilities
- :doc:`_diagrams/08-message-log-structure` - Message logging structure
- :doc:`_diagrams/09-spare-rank-layout` - Spare rank layout and allocation

Workflows
---------

- :doc:`_diagrams/10-checkpoint-timeline` - Checkpoint and commit timeline

Decision Trees
--------------

- :doc:`_diagrams/12-decision-recovery-pattern` - Choosing a recovery pattern
- :doc:`_diagrams/13-troubleshooting-flowchart` - Troubleshooting guide
