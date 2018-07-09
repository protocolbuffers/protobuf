FROM centos:6.9

RUN yum install -y git \
                   tar \
                   wget \
                   make \
                   autoconf \
                   curl-devel \
                   unzip \
                   automake \
                   libtool \
                   glibc-static.i686 \
                   glibc-devel \
                   glibc-devel.i686 \
                   && \
    yum clean all

# Install Java 8
RUN wget -q --no-cookies --no-check-certificate \
    --header "Cookie: gpw_e24=http%3A%2F%2Fwww.oracle.com%2F; oraclelicense=accept-securebackup-cookie" \
    "http://download.oracle.com/otn-pub/java/jdk/8u131-b11/d54c1d3a095b4ff2b6607d096fa80163/jdk-8u131-linux-x64.tar.gz" \
    -O - | tar xz -C /var/local
ENV JAVA_HOME /var/local/jdk1.8.0_131
ENV PATH $JAVA_HOME/bin:$PATH

# Install Maven
RUN wget -q http://apache.cs.utah.edu/maven/maven-3/3.3.9/binaries/apache-maven-3.3.9-bin.tar.gz -O - | \
    tar xz -C /var/local
ENV PATH /var/local/apache-maven-3.3.9/bin:$PATH

# Install GCC 4.8 to support -static-libstdc++
RUN wget http://people.centos.org/tru/devtools-2/devtools-2.repo -P /etc/yum.repos.d && \
    bash -c 'echo "enabled=1" >> /etc/yum.repos.d/devtools-2.repo' && \
    bash -c "sed -e 's/\$basearch/i386/g' /etc/yum.repos.d/devtools-2.repo > /etc/yum.repos.d/devtools-i386-2.repo" && \
    sed -e 's/testing-/testing-i386-/g' -i /etc/yum.repos.d/devtools-i386-2.repo && \
    rpm --rebuilddb && \
    yum install -y devtoolset-2-gcc devtoolset-2-gcc-c++ devtoolset-2-binutils devtoolset-2-libstdc++-devel \
                   devtoolset-2-gcc.i686 devtoolset-2-gcc-c++.i686 devtoolset-2-binutils.i686 devtoolset-2-libstdc++-devel.i686 && \
    yum clean all

COPY scl-enable-devtoolset.sh /var/local/

# Start in devtoolset environment that uses GCC 4.7
ENTRYPOINT ["/var/local/scl-enable-devtoolset.sh"]
