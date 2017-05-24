# docker build -t gr1c:latest -f Dockerfile ..
#
# The tag `gr1c:latest` can be changed or omitted.
#
# After the image is built, the container can replace `gr1c` on the
# command-line unless file names appear as arguments. Specifications
# be provided via stdin and pipes. E.g.,
#
#     cat examples/trivial.spc | docker run -i --rm gr1c:latest
#
# synthesizes according to examples/trivial.spc and removes the Docker
# container afterward because of `--rm`. E.g.,
#
#     docker run -i --rm gr1c:latest help
#
# causes the gr1c help message to be printed.

FROM debian:stable

RUN apt-get update && apt-get -y install \
    curl \
    graphviz \
    build-essential \
    bison \
    flex

COPY . /root/gr1c

WORKDIR /root/gr1c
RUN ./get-deps.sh && ./get-extra-deps.sh
RUN ./build-deps.sh

RUN make -j4 all && make check

ENV PATH /root/gr1c:$PATH

ENTRYPOINT ["gr1c"]
