# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

NEED_GPERFTOOLS=0
BRPC_PATH=../..
include $(BRPC_PATH)/config.mk
# Notes on the flags:
# 1. Added -fno-omit-frame-pointer: perf/tcmalloc-profiler use frame pointers by default
CXXFLAGS+=$(CPPFLAGS) -std=c++17 -DNDEBUG -O2 -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer
ifeq ($(NEED_GPERFTOOLS), 1)
    CXXFLAGS+=-DBRPC_ENABLE_CPU_PROFILER
endif
HDRS+=$(BRPC_PATH)/output/include
LIBS+=$(BRPC_PATH)/output/lib

HDRPATHS=$(addprefix -I, $(HDRS))
LIBPATHS=$(addprefix -L, $(LIBS))
COMMA=,
SOPATHS=$(addprefix -Wl$(COMMA)-rpath$(COMMA), $(LIBS))

# Add platform-specific library paths
ifeq ($(SYSTEM),Darwin)
    # macOS - Homebrew paths
    HDRPATHS += -I/opt/homebrew/include -I/usr/local/include
    LIBPATHS += -L/opt/homebrew/lib -L/usr/local/lib
else ifeq ($(SYSTEM),Linux)
    # Linux - standard paths, make sure to edit these paths to your couchbase-cxx-client library location and fmt library location
    HDRPATHS += -I/usr/local/include -I/usr/include
    LIBPATHS += -L/usr/local/lib -L/usr/lib -L/usr/lib/x86_64-linux-gnu
endif

CLIENT_SOURCES = client.cpp
MULTI_THREADED_SOURCES = multi_threaded_client.cpp
PROTOS = $(wildcard *.proto)

PROTO_OBJS = $(PROTOS:.proto=.pb.o)
PROTO_GENS = $(PROTOS:.proto=.pb.h) $(PROTOS:.proto=.pb.cc)
CLIENT_OBJS = $(addsuffix .o, $(basename $(CLIENT_SOURCES)))
MULTI_THREADED_OBJS = $(addsuffix .o, $(basename $(MULTI_THREADED_SOURCES))) 

# Add couchbase object file from brpc source
COUCHBASE_OBJ = $(BRPC_PATH)/src/brpc/couchbase.o 

ifeq ($(SYSTEM),Darwin)
 ifneq ("$(LINK_SO)", "")
    STATIC_LINKINGS += -lbrpc
 else
    # *.a must be explicitly specified in clang
    STATIC_LINKINGS += $(BRPC_PATH)/output/lib/libbrpc.a
 endif
	LINK_OPTIONS_SO = $^ $(STATIC_LINKINGS) $(DYNAMIC_LINKINGS) -lcouchbase_cxx_client -lfmt
	LINK_OPTIONS = $^ $(STATIC_LINKINGS) $(DYNAMIC_LINKINGS) -lcouchbase_cxx_client -lfmt
else ifeq ($(SYSTEM),Linux)
    STATIC_LINKINGS += -lbrpc
    LINK_OPTIONS_SO = -Xlinker "-(" $^ -Xlinker "-)" $(STATIC_LINKINGS) $(DYNAMIC_LINKINGS) -lcouchbase_cxx_client -lfmt
    LINK_OPTIONS = -Xlinker "-(" $^ -Wl,-Bstatic $(STATIC_LINKINGS) -Wl,-Bdynamic -Xlinker "-)" $(DYNAMIC_LINKINGS) -lcouchbase_cxx_client -lfmt
endif

.PHONY:all clean help test
all: couchbase_client multi_threaded_client

help:
	@echo "Available targets:"
	@echo "  all                     - Build couchbase_client executable"
	@echo "  couchbase_client        - Build the single cluster couchbase_client executable"
	@echo "  clean                   - Remove all generated files"
	@echo "  test                    - Run the single cluster client with default parameters"
	@echo "  help                    - Show this help message"test: couchbase_client
	@echo "> Running couchbase_client test"
	./couchbase_client --help

.PHONY:clean
clean:
	@echo "> Cleaning"
	rm -rf couchbase_client multi_threaded_client $(PROTO_GENS) $(PROTO_OBJS) $(CLIENT_OBJS) $(MULTI_THREADED_OBJS)

couchbase_client:$(PROTO_OBJS) $(CLIENT_OBJS) $(COUCHBASE_OBJ)
	@echo "> Linking $@"
ifneq ("$(LINK_SO)", "")
	$(CXX) $(LIBPATHS) $(SOPATHS) $(LINK_OPTIONS_SO) -o $@
else
	$(CXX) $(LIBPATHS) $(LINK_OPTIONS) -o $@
endif

multi_threaded_client:$(PROTO_OBJS) $(MULTI_THREADED_OBJS) $(COUCHBASE_OBJ)
	@echo "> Linking $@"
ifneq ("$(LINK_SO)", "")
	$(CXX) $(LIBPATHS) $(SOPATHS) $(LINK_OPTIONS_SO) -o $@
else
	$(CXX) $(LIBPATHS) $(LINK_OPTIONS) -o $@
endif

%.pb.cc %.pb.h:%.proto
	@echo "> Generating $@"
	$(PROTOC) --cpp_out=. --proto_path=. $(PROTOBUF_HDR) $<

%.o:%.cpp
	@echo "> Compiling $@"
	$(CXX) -c $(HDRPATHS) $(CXXFLAGS) $< -o $@

%.o:%.cc
	@echo "> Compiling $@"
	$(CXX) -c $(HDRPATHS) $(CXXFLAGS) $< -o $@

%.o:%.c
	@echo "> Compiling $@"
	$(CC) -c $(HDRPATHS) $(CFLAGS) $< -o $@
