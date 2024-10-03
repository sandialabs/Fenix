Functions and types for process recovery.

* Only communicators derived from the communicator returned by 
Fenix_Init are eligible for reconstruction. 
After communicators have been repaired, they contain the same
number of ranks as before the failure occurred, unless the user
did not allocate sufficient redundant resources (*spare ranks*)
and instructed Fenix not to create new ranks. In this case 
communicators will still be repaired, but will contain fewer 
ranks than before the failure occurred.

* To ease adoption of MPI fault tolerance, Fenix automatically
captures any errors resulting from MPI library calls that are a
result of a damaged communicator (other errors reported by the
MPI runtime are ignored by Fenix and are returned to the 
application, for handling by the application writer). In other
words, programmers do not need to replace calls to the MPI library
with calls to Fenix (for example, *Fenix_Send* instead of 
*MPI_Send*).
