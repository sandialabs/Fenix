Fenix provides options for redundant storage of application data 
to facilitate application data recovery in a transparent manner.
Fenix contains functions to control consistency of collections of
such data, as well as their level of persistence. Functions with
the prefix \c Fenix\_Data\_ perform store, versioning, restore,
and other relevant operations and form the Fenix data recovery API.
The user can select a specific set of application data, identified
by its location in memory, label it using [Fenix_Data_member_create](@ref Fenix_Data_member_create),
and copy it into Fenix's redundant storage space through 
[Fenix_Data_member(i)store(v)](@ref Fenix_Data_member_store) at a 
point in time. Subsequently, #Fenix_Data_commit finalizes all 
preceding Fenix store operations involving this data group and 
assigns a unique time stamp to the resulting data *snapshot*, 
marking the data as potentially recoverable after a loss of ranks. 
Individual pieces of data can then be restored whenever they are 
needed with #Fenix_Data_member_restore, for example after a failure 
occurs. We note that Fenix's data storage and recovery facility 
aims primarily to support in-memory recovery.

Populating redundant data storage using Fenix may involve the 
dispersion of data created by one rank to other ranks within the
system, making the store operation semantically a collective
operation. However, Fenix does not require store operations to be
globally synchronizing. For example, execution of 
 #Fenix_Data_member_store for a particular collection of data 
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

-----

## Data Groups

A Fenix *data group* provides dual functionality. First, it serves
as a container for a set of data objects (*members*) that are
committed together, and hence provides transaction semantics.
Second, it recognizes that #Fenix_Data_member_store is an operation 
carried out collectively by a group of ranks, but not necessarily 
by all active ranks in the MPI environment. Hence, it adopts the 
convenient MPI vehicle of \c communicators to indicate the subset 
of ranks involved. Data groups are composed of members that 
describe the actual application data and the redundancy policy
to be used for securely storing the members.

Data groups can and should be recreated after each failure (i.e. do not
conditionally skip the creation after initialization).

See #Fenix_Data_group_create
for creating a data group.

-----

## Data Redundancy Policies

Fenix internally uses an extensible system for defining data 
policies to keep the door open to easily adding new data policies
and configuring them on a per-data-group basis. We currently
support a single, configurable, memory-based policy.

### In Memory Redundancy Policy (IMR)

IMR is referenced with the FENIX_DATA_POLICY_IN_MEMORY_RAID definition, 
and takes as input an array of integers with the following usage:

* Mode: (1 or 5) Chooses storage mimicking the given RAID style.
* Separation: Sets the rank separation for groups used to store redundant data.
   Users should choose a separation that attempts to ensure the ranks 
   chosen for grouping are not colocated on nodes/racks to minimize the
   chance of multiple ranks in a group 
* GroupSize: For Mode 5 only, sets the size of the parity groups, minimum 3.

The policy is designed to localize recovery as much as possible. Communication
amongst group members is required (as failure during recovery operations
can lead to inconsistent beliefs about which ranks have recovered data),
but groups without recovering ranks may then all recover locally rather
than communicating further. Groups need not wait for ranks outside of 
their group to enter or exit recovery.

* **Mode 1**: Groups ranks into dyadically paired partners of Rank N and 
          Rank (N+Separation). For odd-size communicators, a single 
          group of size 3 will also form of the first, middle, and last 
          ranks. Each rank stores a copy of its own data and a copy of 
          its partner's. For groups of three, partner data storage is 
          chained. Should both partners fail (or any two for groups of 
          three) before recovery operations have completed, data will be 
          unrecoverable. 
          
  **Memory Usage**: Each rank stores a copy of its own data and of its 
          partner's data for each timestamp, where checkpoint depth D
          stores D+1 checkpoints. Therefore for data size M, 
          (D+1)*M*2 bytes are used.

  **Computation**: None.

* **Mode 5**: Groups ranks into parity groups of size GroupSize.
          Groups are formed of Rank N, N+Separation, N+2*Separation.
          If any two ranks in a group fail before recovery operations 
          have completed, data will be unrecoverable.

  **Memory Usage**: Each rank stores a copy of its own data and 
          M/(GroupSize-1) parity bytes per timestamp. Therefore,
          (D+1)*M*(GroupSize/(GroupSize-1)) bytes are used.

  **Computation**: O(M) parity bit calculations.

These options enable users to trade reliability and computation for memory 
space, which may be necessary for applications with large memory usage.

