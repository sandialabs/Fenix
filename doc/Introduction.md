Fenix is a software library compatible with the Message Passing
Interface (MPI) to support fault recovery without application
shutdown. Fenix has two components: process recovery and data
recovery. Process recovery is used to repair communicators whose
ranks suffered failure detected by the MPI runtime. Data recovery
is an optional feature that can be used to implement a 
high-performance in-memory checkpoint/restart mechanism.

Below is a brief overview of these two components, but see the
[Process Recovery](@ref ProcessRecovery) and [Data Recovery](@ref DataRecovery)
topics for more details.

## Process Recovery

The core feature of process recovery is creation of a resilient
communicator that will automatically repair itself. This recovery
is achieved by setting aside some number of ranks as *spare ranks*.
When a failure is detected, the spare ranks are used to replace
the failed ranks.

The exact process of recovery is subject to some nuances of the OpenMPI
ULFM specification, which Fenix is implemented on top of. For example,
messages may have locally succeeded while failing on other participating
ranks.

![An example process flow diagram for recovery using Fenix](fenix_process_flow.png){html: width=300px}

The default recovery pattern is to perform a `longjmp` to the location of
#Fenix_Init following communicator repairs. This emulates the typical offline
checkpoint/restart pattern, but without the need to restart the application.
However, `longjmp` has some nebulous behavior in many applications. Fenix also
supports a non-jumping recovery pattern. This is more predictable across compilers
and optimizations, but requires checking the return value of every MPI call to
detect failed operations (though communicator repair is still automatic). A
good practice for C++ applications is to use the non-jumping pattern, but add
a Fenix error-handler callback to throw an exception on failure.

## Data Recovery

Fenix provides its own redundant data storage API to facilitate 
data recovery along with process recovery, but the user can choose
other data recovery options to meet a variety of application needs.
For example, data could be recovered by approximately interpolating 
values from unaffected, topologically neighboring ranks instead of
by reading stored redundant data. In addition, the user may decide
to use external libraries such as 
[VeloC](https://veloc.readthedocs.io/en/latest/).

> Any Fenix function without a return type, e.g. #Fenix_Init, may be 
> implemented via macros, in which case it cannot be used to resolve
> function pointers.
