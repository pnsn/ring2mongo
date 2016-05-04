FROM joncon/eworm-64:7.8
MAINTAINER joncon@uw.edu

RUN yum install -y pkgconfig \
                   openssl-devel \
                   cyrus-sasl-devel \
                   wget \
                   gcc \
                   perl \
                   make \
                   gdb \
                   valgrind \
                      
                   
RUN wget https://github.com/mongodb/mongo-c-driver/releases/download/1.3.0/mongo-c-driver-1.3.0.tar.gz \
  && tar xzf mongo-c-driver-1.3.0.tar.gz --no-same-owner \
  && cd mongo-c-driver-1.3.0 \
  && ./configure \
  && make \
  && make install
  
  RUN  echo "export CPATH=/usr/local/include/libmongoc-1.0:/usr/local/include/libbson-1.0" >> ~/.bashrc
  RUN  echo "export LD_LIBRARY_PATH=/usr/local/lib" >> ~/.bashrc
  RUN  echo "export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig" >> ~/.bashrc