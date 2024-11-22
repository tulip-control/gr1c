# docker build -t gr1c:latest -f Dockerfile ..
#
# The tag `gr1c:latest` can be changed or omitted.
#
# Run `make clean` to remove any existing executable or object files
# before building this image.
#
# After the image is built, the container can replace `gr1c` on the
# command-line unless file names appear as arguments. Specifications
# be provided via stdin and pipes. E.g.,
#
#     cat examples/trivial.spc | docker run -i --rm gr1c:latest gr1c
#
# synthesizes according to examples/trivial.spc and removes the Docker
# container afterward because of `--rm`. E.g.,
#
#     docker run -i --rm gr1c:latest gr1c help
#
# causes the gr1c help message to be printed. To drop into a shell with
# gr1c installed,
#
#     docker run -it --rm gr1c:latest bash


FROM debian:stable

RUN apt-get update && apt-get -y install \
    curl \
    graphviz \
    build-essential \
    bison \
    flex \
    python3

COPY . /root/gr1c

WORKDIR /root/gr1c
RUN ./get-deps.sh && ./get-extra-deps.sh
RUN ./build-deps.sh

RUN make -j4 all && make check

ENV PATH /root/gr1c:$PATH

CMD ["gr1c"]
