#!/bin/bash

if [ -f ulfm-install/lib/libmpi.so ]; then
  echo "libmpich.so found -- nothing to build."
  cd ulfm-install
else
  ROOT=`pwd`
  echo "Downloading ULFM from repo"
  wget https://bitbucket.org/icldistcomp/ulfm2/get/ulfm2.0rc.tar.bz2
  tar -xjf ulfm2.0rc.tar.bz2
  mv icldist* ulfm-src/
  echo " - Configuring and building ULFM."
  cd ulfm-src
  echo " - Running autogen.pl"
  ./autogen.pl > ../build_output.txt
  echo " - Running configure"
  ./configure --prefix=$ROOT/ulfm-install >> ../build_output.txt
  echo " - Running make"
  make -j4 >> ../build_output.txt
  echo " - Running make install"
  make install >> ../build_output.txt
  echo " - Finished installing ULFM"
  cd ../ulfm-install/
fi

#Expect that any changes to the above still puts me in the install's home dir
export MPI_HOME=`pwd`
export PATH=$MPI_HOME/bin/:$PATH
export LD_LIBRARY_PATH=$MPI_HOME/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$MPI_HOME/lib:$DYLD_LIBRARY_PATH
export MANPATH=$MPI_HOME/share/man:$MANPATH

export MPICC="`which mpicc`"
export MPICXX="`which mpic++`"

#Assuming the install's home dir is one above current.
cd ../
