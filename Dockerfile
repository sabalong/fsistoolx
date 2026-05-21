ARG GO_VER="1.22-suse"
ARG DEPS_IMAGE="your-registry/fsis-deps:latest"

# Stage 0: Build dependencies (GLib, GSL, libxml2, libxslt, TinyCC)
FROM registry.suse.com/bci/gcc:15 AS deps-builder

# Install build dependencies
RUN zypper --non-interactive ref && \
    zypper --non-interactive install \
      git \
    go \
      meson \
      ninja \
      pkg-config \
      gettext-tools \
      libffi-devel \
      pcre2-devel \
      python3 \
      python3-pip \
      glibc-devel \
      gobject-introspection-devel \
      make \
      autoconf \
      automake \
      libtool \
      wget \
      unzip \
      python3-packaging \
      tar \
      glibc-devel \
      xz \
      cmake \
      Xerces-c \
      boost-devel \
      gtest && \
    zypper clean

WORKDIR /build

# Fetch and build GLib
RUN wget https://download.gnome.org/sources/glib/2.80/glib-2.80.0.tar.xz && \
    tar -xf glib-2.80.0.tar.xz && \
    cd glib-2.80.0 && \
    meson setup builddir --prefix=/opt/glib \
        -Dintrospection=disabled \
        -Ddocumentation=false && \
    ninja -C builddir && \
    ninja -C builddir install

# Build libxml2
WORKDIR /build
RUN wget https://download.gnome.org/sources/libxml2/2.12/libxml2-2.12.7.tar.xz && \
    tar -xf libxml2-2.12.7.tar.xz && \
    cd libxml2-2.12.7 && \
    ./configure --prefix=/opt/libxml2 --without-python && \
    make -j$(nproc) && \
    make install

# Build libxslt (depends on libxml2)
WORKDIR /build
RUN wget https://download.gnome.org/sources/libxslt/1.1/libxslt-1.1.39.tar.xz && \
    tar -xf libxslt-1.1.39.tar.xz && \
    cd libxslt-1.1.39 && \
    ./configure --prefix=/opt/libxslt --with-libxml-prefix=/opt/libxml2 --without-python && \
    make -j$(nproc) && \
    make install

# Build GSL (version 2.8 for libgsl.so.28)
WORKDIR /build
RUN wget https://ftp.gnu.org/gnu/gsl/gsl-2.8.tar.gz && \
    tar -xf gsl-2.8.tar.gz && \
    cd gsl-2.8 && \
    ./configure --prefix=/opt/gsl && \
    make -j$(nproc) && \
    make install

# Build TinyCC
WORKDIR /build
RUN curl -LO https://github.com/TinyCC/tinycc/archive/refs/heads/mob.zip && \
    unzip mob.zip && \
    cd tinycc-mob && \
    ./configure --prefix=/usr/local --enable-static && \
    make -j$(nproc) && \
    make install && \
    cp /usr/local/lib/tcc/libtcc1.a /usr/local/lib/

# Build ANTLR4 C++ Runtime
WORKDIR /build
RUN wget https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip && \
    unzip antlr4-cpp-runtime-4.13.2-source.zip -d antlr4-cpp && \
    cd antlr4-cpp && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) && \
    make install

# Build xerces-c from source
WORKDIR /build
RUN wget https://dlcdn.apache.org//xerces/c/3/sources/xerces-c-3.3.0.tar.gz && \
    tar -xzf xerces-c-3.3.0.tar.gz && \
    cd xerces-c-3.3.0 && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install


    
# Set environment variables for building with dependencies
ENV PKG_CONFIG_PATH=/opt/glib/lib64/pkgconfig:/opt/libxml2/lib/pkgconfig:/opt/libxslt/lib/pkgconfig:/opt/gsl/lib/pkgconfig:${PKG_CONFIG_PATH:-}
ENV LD_LIBRARY_PATH=/opt/glib/lib64:/opt/libxml2/lib:/opt/libxslt/lib:/opt/gsl/lib:/usr/local/lib:${LD_LIBRARY_PATH:-}
ENV CFLAGS="-I/opt/glib/include/glib-2.0 -I/opt/glib/lib64/glib-2.0/include -I/opt/libxml2/include/libxml2 -I/opt/libxslt/include -I/opt/gsl/include"
ENV LDFLAGS="-L/opt/glib/lib64 -L/opt/libxml2/lib -L/opt/libxslt/lib -L/opt/gsl/lib"

ENV XERCES_HOME=/usr/local
ENV CMAKE_PREFIX_PATH=/usr/local:${CMAKE_PREFIX_PATH:-}

COPY ./tools/XQilla-2.3.4 /XQilla-2.3.4
RUN cd /XQilla-2.3.4 && \
    rm -rf .libs && \
    rm -f compile-delayed-module compile-delayed-module.o && \
    ./configure --prefix=/usr/local --with-xerces=/usr/local && \
    find ./src/functions -name '*.xq' -exec touch -t 200001010000 {} \; && \
    find ./src/functions -name '*.hpp' -exec touch {} \; && \
    make -j$(nproc) && \
    make install

COPY ./ilfreporter-0.0.1 /ilfreporter-0.0.1
RUN cd /ilfreporter-0.0.1 && \
    rm -rf build && \
    mkdir -p build && cd build && \
    cmake -DCMAKE_PREFIX_PATH=/usr/local .. && \
    make -j
  
COPY ./ilf /ilf

RUN cd /ilf && \
    make clean && \
    make kpmr_source_tree_cli && \
    make build_tree_kpmr_cli && \
    make eval_cli && \
    make eval_server && \
    make build_tree_cli

# Copy libgofunct before building ilfx (required for linking)

COPY ./gofunct /gofunct
COPY go.mod go.mod
COPY go.sum go.sum
COPY ./vendor vendor/

RUN go build -buildmode=c-archive -mod=vendor -o /gofunct/gen/libgofunct.a /gofunct/main.go

RUN cp /gofunct/gen/libgofunct.a /usr/local/lib/libgofunct.a

# Build ilfx project
COPY ./ilfx /ilfx

RUN cd /ilfx && \
    rm -rf build && \
    mkdir -p build && \
    cd build && \
    cmake \
      -DCMAKE_PREFIX_PATH=/usr/local \
      -DCMAKE_C_FLAGS="-I/opt/gsl/include" \
      -DCMAKE_CXX_FLAGS="-I/opt/gsl/include" \
      -DCMAKE_EXE_LINKER_FLAGS="-L/opt/gsl/lib" \
      .. && \
    make

COPY ./xsltcli /xsltcli

RUN cd /xsltcli && \
    export PATH="/opt/libxml2/bin:/opt/libxslt/bin:${PATH}" && \
    make clean && \
    make build

# Stage 0b: Export dependency bundle artifact
FROM registry.suse.com/bci/bci-base:15.7 AS deps-bundle

COPY --from=deps-builder /opt/glib /opt/glib
COPY --from=deps-builder /opt/gsl /opt/gsl
COPY --from=deps-builder /opt/libxml2 /opt/libxml2
COPY --from=deps-builder /opt/libxslt /opt/libxslt
COPY --from=deps-builder /usr/local /usr/local
COPY --from=deps-builder /usr/include /usr/include
COPY --from=deps-builder /usr/lib64 /usr/lib64
COPY --from=deps-builder /ilf/kpmr_source_tree_cli /ilf/kpmr_source_tree_cli
COPY --from=deps-builder /ilf/eval_cli /ilf/eval_cli
COPY --from=deps-builder /ilf/eval_server /ilf/eval_server
COPY --from=deps-builder /ilf/build_tree_cli /ilf/build_tree_cli
COPY --from=deps-builder /ilf/build_tree_kpmr_cli /ilf/build_tree_kpmr_cli
COPY --from=deps-builder /ilfx/build/bin /ilfx/build/bin
COPY --from=deps-builder /ilfreporter-0.0.1/build/ilfreporter /ilfreporter-0.0.1/build/ilfreporter
COPY --from=deps-builder /ilfreporter-0.0.1/build/kpmr_cli /ilfreporter-0.0.1/build/kpmr_cli
COPY --from=deps-builder /xsltcli/xsltcli /xsltcli/xsltcli

RUN tar -czf /deps-bundle.tar.gz \
    /opt/glib \
    /opt/gsl \
    /opt/libxml2 \
    /opt/libxslt \
    /usr/local \
    /usr/include \
    /usr/lib64 \
    /ilf \
    /ilfx/build/bin \
    /ilfreporter-0.0.1/build \
    /xsltcli/xsltcli
