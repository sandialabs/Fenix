#Built for testing, not designed for application use.

FROM ubuntu:20.04
#="open-mpi/ompi" for github.com/open-mpi/ompi
ARG OPENMPI_REPO="open-mpi/ompi"
#="tags" or ="heads", for tag or branch name
ARG OPENMPI_VERS_PREFIX="tags"
#="v5.0.0rc10" or ="v5.0.x", ie tag name or branch name.
ARG OPENMPI_VERS="v5.0.0rc10"
run echo Using https://github.com/${OPENMPI_REPO}/git/refs/${OPENMPI_VERS_PREFIX}/${OPENMPI_VERS}

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential python3 m4 autoconf automake libtool flex git zlib1g-dev

#Add files listing latest commit for this branch/tag, which invalidates the clone
#when a change has been pushed.
ADD https://api.github.com/repos/${OPENMPI_REPO}/git/refs/${OPENMPI_VERS_PREFIX}/${OPENMPI_VERS} commit_info
RUN git clone --recursive --branch ${OPENMPI_VERS} --depth 1 https://github.com/${OPENMPI_REPO}.git ompi_src && \
    mkdir ompi_build ompi_install && cd ompi_src && export AUTOMAKE_JOBS=8 && ./autogen.pl && cd ../ompi_build && ../ompi_src/configure --prefix=/ompi_install --disable-man-pages --with-ft=ulfm && make install -j8 && cd ..


#New build stage, tosses out src/build trees from openmpi
FROM ubuntu:20.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake ssh zlib1g-dev
COPY . ./fenix_src
COPY --from=0 ompi_install/ /ompi_install/
ENV PATH="$PATH:/ompi_install/bin"
RUN mkdir fenix_build fenix_install && cd fenix_build && cmake ../fenix_src -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/ompi_install/bin/mpicc \
    -DFENIX_EXAMPLES=ON -DFENIX_TESTS=ON -DCMAKE_INSTALL_PREFIX=../fenix_install -DMPIEXEC_PREFLAGS="--allow-run-as-root;--map-by;:OVERSUBSCRIBE" && make install -j8
CMD ["sh", "-c", "cd fenix_build && ctest --verbose --timeout 60"]
