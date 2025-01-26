ARCH = $(shell uname -m)
ifeq ($(ARCH), aarch64)
CXX = aarch64-linux-gnu-g++
else
CXX = g++
endif
CXXFLAGS = -fPIC -fstack-protector-all -Wl,-z,relro,-z,now
SHARED = -shared
TARGET = libeSDKLogAPI.so
${warning spdlog_dir is ${spdlog_dir}}
INCLUDEDIR = -I ../../../platform/libboundscheck/include -I ../output/include -I ../ -I ../vos -I ../../../../${spdlog_dir}/include

../../../platform/libboundscheck/src/%.o : ../../../platform/libboundscheck/src/%.c
	$(CXX) $(CXXFLAGS) $(INCLUDEDIR) -c -DENV_LINUX -DLOG4CPP_HAVE_SSTREAM $< -o $@ 

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDEDIR) -c -DENV_LINUX -DLOG4CPP_HAVE_SSTREAM $< -o $@

SOURCES = $(wildcard *.cpp) $(wildcard ./../vos/*.cpp) 
OBJS = $(patsubst %.cpp,%.o,$(SOURCES))
SECUREC_SOURCES = $(wildcard ../../../platform/libboundscheck/src/*.c)
SECUREC_OBJS = $(patsubst %.c,%.o,$(SECUREC_SOURCES))
OBJS += $(SECUREC_OBJS)

$(TARGET): $(OBJS)
ifeq ($(ARCH), aarch64)
	$(CXX) $(CXXFLAGS) $(SHARED) -DDEBUG -DENV_LINUX $^ -L../../../../${spdlog_dir}/lib -lspdlog -o $(TARGET) -lpthread
else
	$(CXX) $(CXXFLAGS) $(SHARED) -DDEBUG -DENV_LINUX $^ -L../../../../${spdlog_dir}/lib -lspdlog -o $(TARGET) -lpthread
endif

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

