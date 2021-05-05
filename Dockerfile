FROM alpine:3.12


WORKDIR /src
COPY . .

RUN apk add autoconf automake libtool curl make g++ unzip bash
RUN ./autogen.sh
RUN ./configure
RUN make
#RUN make check
#RUN sudo make install

# refresh shared library cache.
#RUN sudo ldconfig 