FROM centos:6.6

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
                   glibc-devel.i686

# Install Java 8
RUN wget -q --no-cookies --no-check-certificate \
    --header "Cookie: gpw_e24=http%3A%2F%2Fwww.oracle.com%2F; oraclelicense=accept-securebackup-cookie" "http://download.oracle.com/otn-pub/java/jdk/8u45-b14/jdk-8u45-linux-x64.tar.gz" \
    -O - | tar xz -C /var/local
ENV JAVA_HOME /var/local/jdk1.8.0_45
ENV PATH $JAVA_HOME/bin:$PATH

# Install Maven
RUN wget -q http://apache.cs.utah.edu/maven/maven-3/3.3.9/binaries/apache-maven-3.3.9-bin.tar.gz -O - | \
    tar xz -C /var/local
ENV PATH /var/local/apache-maven-3.3.9/bin:$PATH

# Install GCC 4.7 to support -static-libstdc++
RUN wget http://people.centos.org/tru/devtools-1.1/devtools-1.1.repo -P /etc/yum.repos.d
RUN bash -c 'echo "enabled=1" >> /etc/yum.repos.d/devtools-1.1.repo'
RUN bash -c "sed -e 's/\$basearch/i386/g' /etc/yum.repos.d/devtools-1.1.repo > /etc/yum.repos.d/devtools-i386-1.1.repo"
RUN sed -e 's/testing-/testing-i386-/g' -i /etc/yum.repos.d/devtools-i386-1.1.repo
RUN yum install -y devtoolset-1.1 \
                   devtoolset-1.1-libstdc++-devel \
                   devtoolset-1.1-libstdc++-devel.i686

RUN git clone --depth 1 https://github.com/google/protobuf.git

# Start in devtoolset environment that uses GCC 4.7
CMD ["scl", "enable", "devtoolset-1.1", "bash"]
