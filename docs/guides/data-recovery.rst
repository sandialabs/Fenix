Data Recovery
=============

Fenix provides options for redundant storage of application data
to facilitate application data recovery in a transparent manner.
Fenix contains functions to control consistency of collections of
such data, as well as their level of persistence. Functions with
the prefix ``Fenix_Data_`` perform store, versioning, restore,
and other relevant operations and form the Fenix data recovery API.
The user can select a specific set of application data, identified
by its location in memory, label it using :c:func:`Fenix_Data_member_create`,
and copy it into Fenix's redundant storage space through
:c:func:`Fenix_Data_member_store` at a
point in time. Subsequently, :c:func:`Fenix_Data_commit` finalizes all
preceding Fenix store operations involving this data group and
assigns a unique time stamp to the resulting data *snapshot*,
marking the data as potentially recoverable after a loss of ranks.
Individual pieces of data can then be restored whenever they are
needed with :c:func:`Fenix_Data_member_restore`, for example after a failure
occurs. We note that Fenix's data storage and recovery facility
aims primarily to support in-memory recovery.

Populating redundant data storage using Fenix may involve the
dispersion of data created by one rank to other ranks within the
system, making the store operation semantically a collective
operation (all ranks in the data group must call it). However, Fenix
does not require store operations to be globally synchronizing - they
do not act as barriers. For example, execution of
:c:func:`Fenix_Data_member_store` for a particular member could
potentially complete on some ranks while still in progress on others.
If certain ranks nominally participating in the storage operations
have no actual data movement responsibility (based on the redundancy
policy), Fenix is allowed to let them exit the operation immediately.
Consequently, Fenix data storage functions should not be used for
synchronization purposes - use ``MPI_Barrier`` if you need
synchronization.

Multiple distinct pieces (members) of data assigned to Fenix-managed
redundant storage can be associated with a specific instance of
a Fenix *data group* to form a semantic unit (transaction).
Committing such a group ensures that all members in the group are
stored atomically - either all are available for recovery, or none are.
This provides transaction semantics for checkpoint consistency.

----

Data Groups
-----------

A Fenix *data group* provides dual functionality. First, it serves
as a container for a set of data objects (*members*) that are
committed together, providing transaction semantics - either all
members in the group are committed atomically, or none are.
Second, it recognizes that :c:func:`Fenix_Data_member_store` is an operation
carried out collectively by a group of ranks, but not necessarily
by all active ranks in the MPI environment. Hence, it uses an MPI
communicator to define the subset of ranks participating in the
group. For example, if only ranks 0-99 need to checkpoint certain
data, you can create a data group with a sub-communicator containing
just those ranks. Data groups are composed of members (describing
the actual application data) and a redundancy policy (describing
how to store members securely across ranks).

Data groups can and should be recreated after each failure (i.e. do not
conditionally skip the creation after initialization).

See :c:func:`Fenix_Data_group_create`
for creating a data group.

----

Data Redundancy Policies
-------------------------

Fenix internally uses an extensible system for defining data
policies to keep the door open to easily adding new data policies
and configuring them on a per-data-group basis. We currently
support a single, configurable, memory-based policy.

See :doc:`imr-policy` for details on the In Memory Redundancy Policy (IMR).

----

See Also
--------

- :doc:`../tutorials/02-data-recovery` - Tutorial on adding data recovery
- :doc:`../howto/checkpoint-data` - How-to guide for checkpointing data
- :doc:`../howto/partial-checkpoints` - Checkpoint only part of arrays
- :doc:`../api/data-recovery` - Data recovery API reference
- :doc:`imr-policy` - In-Memory Redundancy policy details
- :doc:`architecture` - Overall Fenix architecture
