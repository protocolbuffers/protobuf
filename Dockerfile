FROM alpine:3.12

RUN apk add autoconf automake libtool curl make g++ unzip bash

WORKDIR /src

COPY . .
RUN ./autogen.sh
RUN ./configure
RUN make -j 4

#check on alpine will no pass
#RUN make check
RUN make install

# refresh shared library cache.
#RUN ldconfig 

ENTRYPOINT ["/usr/local/bin/protoc"]