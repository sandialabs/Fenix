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
operation. However, Fenix does not require store operations to be
globally synchronizing. For example, execution of
:c:func:`Fenix_Data_member_store` for a particular collection of data
could potentially be finished in some ranks, but not yet in others.
And if certain ranks nominally participating in the storage
operations have no actual data movement responsibility, Fenix is
allowerd to let them exit the operation immediately. Consequently,
Fenix data storage functions should not be used for synchronization
purposes.

Multiple distinct pieces (members) of data assigned to Fenix-managed
redundant storage, can be associated with a specific instance of
a Fenix *data group* to form a semantic unit. Committing such a
group ensures that the data involved is available for recovery.

----

Data Groups
-----------

A Fenix *data group* provides dual functionality. First, it serves
as a container for a set of data objects (*members*) that are
committed together, and hence provides transaction semantics.
Second, it recognizes that :c:func:`Fenix_Data_member_store` is an operation
carried out collectively by a group of ranks, but not necessarily
by all active ranks in the MPI environment. Hence, it adopts the
convenient MPI vehicle of ``communicators`` to indicate the subset
of ranks involved. Data groups are composed of members that
describe the actual application data and the redundancy policy
to be used for securely storing the members.

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
