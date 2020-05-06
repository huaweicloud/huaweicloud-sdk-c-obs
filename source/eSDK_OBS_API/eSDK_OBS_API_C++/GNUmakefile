# --------------------------------------------------------------------------
# Set libeSDKOBS version number, unless it is already set.

LIBOBS_VER_MAJOR ?= 2.1
LIBOBS_VER_MINOR ?= 15
LIBOBS_VER := $(LIBOBS_VER_MAJOR).$(LIBOBS_VER_MINOR)

#DEBUG=debug
# -----------------------------------------------------------------------------
# Determine verbosity.  VERBOSE_SHOW should be prepended to every command which
# should only be displayed if VERBOSE is set.  QUIET_ECHO may be used to
# echo text only if VERBOSE is not set.  Typically, a VERBOSE_SHOW command will
# be paired with a QUIET_ECHO command, to provide a command which is displayed
# in VERBOSE mode, along with text which is displayed in non-VERBOSE mode to
# describe the command.
#
# No matter what VERBOSE is defined to, it ends up as true if it's defined.
# This will be weird if you defined VERBOSE=false in the environment, and we
# switch it to true here; but the meaning of VERBOSE is, "if it's defined to
# any value, then verbosity is turned on".  So don't define VERBOSE if you
# don't want verbosity in the build process.
# -----------------------------------------------------------------------------

ifdef VERBOSE
        VERBOSE = true
        VERBOSE_ECHO = @ echo
        VERBOSE_SHOW =
        QUIET_ECHO = @ echo > /dev/null
else
        VERBOSE = false
        VERBOSE_ECHO = @ echo > /dev/null
        VERBOSE_SHOW = @
        QUIET_ECHO = @ echo
endif


# --------------------------------------------------------------------------
# BUILD directory
ifndef BUILD
    ifdef DEBUG
        BUILD := build-debug
    else
        BUILD := build
    endif

 endif


# --------------------------------------------------------------------------
# DESTDIR directory
ifndef DESTDIR
    DESTDIR := /usr
endif

# --------------------------------------------------------------------------
# LIBDIR directory
ifndef LIBDIR
    LIBDIR := ${DESTDIR}/lib
endif

# --------------------------------------------------------------------------
# Compiler CC handling
#ifndef CC
#    CC := gcc
#endif

ifdef BUILD_FOR_ARM
   CC := gcc 
   libPath := aarch64
   includePath := arm
else
   ifdef BUILD_FOR_NDK_AARCH64
      CC := aarch64-linux-android-gcc
      libPath := ndk-aarch64
      includePath := ndk-aarch64
   else
      CC := gcc
      ifdef BUILD_FOR_MACOS
          libPath := macos
          includePath := macos
      else
          libPath := linux_64
          includePath := linux
      endif
   endif
endif

ifndef openssl_version
   openssl_version := openssl-1.1.1d
endif

ifndef curl_version
   curl_version := curl-7.66.0
endif

ifndef nghttp2_version
#   nghttp2_version := nghttp2-1.32.0
endif

ifndef libxml2_version
   libxml2_version := libxml2-2.9.9
endif
# --------------------------------------------------------------------------
# Acquire configuration information for libraries that libeSDKOBS depends upon
export LD_LIBRARY_PATH=./../../../build/script/Provider/build/${includePath}/${openssl_version}/lib

ifndef NGHTTP2_LIBS
#    NGHTTP2_LIBS := -L./../../../build/script/Provider/build/${includePath}/${nghttp2_version}/lib -lnghttp2
endif

ifndef NGHTTP2_CFLAGS
#    NGHTTP2_CFLAGS := -I./../../../build/script/Provider/build/${includePath}/${nghttp2_version}/include
endif

ifndef CURL_LIBS
    CURL_LIBS := -L./../../../build/script/Provider/build/${includePath}/${curl_version}/lib -lcurl
endif

ifndef CURL_CFLAGS
    CURL_CFLAGS := -I./../../../build/script/Provider/build/${includePath}/${curl_version}/include
endif

ifndef LIBXML2_LIBS
    LIBXML2_LIBS := -L./../../../build/script/Provider/build/${includePath}/${libxml2_version}/lib -lxml2 -lm -ldl 
endif

ifndef LIBXML2_CFLAGS
    LIBXML2_CFLAGS := -I./../../../build/script/Provider/build/${includePath}/${libxml2_version}/include
endif

ifndef LIBESDKLOGAPI_CFLAGS
    LIBESDKLOGAPI_CFLAGS := -I./../../../platform/eSDK_LogAPI_V2.1.10/C/include
endif

ifndef LIBESDKLOGAPI_LIBS
    LIBESDKLOGAPI_LIBS := -L./../../../platform/eSDK_LogAPI_V2.1.10/C/${libPath} -leSDKLogAPI
endif

ifndef LIBOPENSSL_LIBS
	LIBOPENSSL_LIBS := -L./../../../build/script/Provider/build/${includePath}/${openssl_version}/lib -lssl -lcrypto
#LIBOPENSSL_LIBS := -L/usr/local/ssl/lib -lssl -lcrypto
#	LIBOPENSSL_LIBS := -L/arm/cross_compile/install/sysroot/lib64 -lssl -lcrypto
endif

ifndef LIBOPENSSL_CFLAGS
     LIBOPENSSL_CFLAGS := -I./../../../build/script/Provider/build/${includePath}/${openssl_version}/include
endif

ifndef LIBPCRE_LIBS
     LIBPCRE_LIBS := -L./../../../build/script/Provider/build/${includePath}/pcre-8.39/lib -lpcre
endif

ifndef LIBPCRE_CFLAGS
     LIBPCRE_CFLAGS := -I./../../../build/script/Provider/build/${includePath}/pcre-8.39/include/pcre
endif

# add libsecurec by jwx329074 2016.09.18
ifndef LIBSECUREC_LIBS
     LIBSECUREC_LIBS := -L./../../../platform/huaweisecurec/lib -lsecurec
endif

ifndef LIBSECUREC_CFLAGS
     LIBSECUREC_CFLAGS := -I./../../../platform/huaweisecurec/include
endif

ifndef LIBICONV_CFLAGS
     ifndef BUILD_FOR_MACOS
         ifdef BUILD_FOR_NDK_AARCH64
         LIBICONV_CFLAGS := -I./../../../build/script/Provider/build/${includePath}/iconv-1.15/include
         else
         LIBICONV_CFLAGS := -I./../../../third_party_groupware/eSDK_Storage_Plugins/libiconv-1.15/source/include
         endif
     endif
endif

ifndef LIBICONV_LIBS
    ifdef BUILD_FOR_MACOS
     LIBICONV_LIBS := -liconv
    else
     LIBICONV_LIBS := -L./../../../build/script/Provider/build/${includePath}/iconv-1.15/lib -liconv
    endif
endif

# --------------------------------------------------------------------------
# These CFLAGS assume a GNU compiler.  For other compilers, write a script
# which converts these arguments into their equivalent for that particular
# compiler.

ifndef CFLAGS
    ifdef DEBUG
        CFLAGS := -ggdb -O0
    else
        CFLAGS := -O3
    endif
endif

ifdef BUILD_FOR_NDK_AARCH64
SPEC_PARAM = -Wno-unused-command-line-argument
endif
ifdef BUILD_FOR_MACOS
SPEC_PARAM = -Wno-unused-command-line-argument -Wno-format
endif
CFLAGS += -Wall -Werror -Wshadow -Wextra -Iinc \
          $(NGHTTP2_CFLAGS) $(CURL_CFLAGS) $(LIBXML2_CFLAGS) $(LIBESDKLOGAPI_CFLAGS) $(LIBOPENSSL_CFLAGS) $(LIBPCRE_CFLAGS) $(LIBSSH2_CFLAGS) $(LIBSECUREC_CFLAGS) $(LIBICONV_CFLAGS)\
          -DLIBOBS_VER_MAJOR=\"$(LIBOBS_VER_MAJOR)\" \
          -DLIBOBS_VER_MINOR=\"$(LIBOBS_VER_MINOR)\" \
          -DLIBOBS_VER=\"$(LIBOBS_VER)\" \
          -D__STRICT_ANSI__ \
          -D_ISOC99_SOURCE \
          -D_POSIX_C_SOURCE=200112L \
		  -std=c99 \
		  -Wno-char-subscripts -Wno-implicit-fallthrough $(SPEC_PARAM)

ifndef BUILD_FOR_NDK_AARCH64
PTHREAD = -lpthread
endif

LDFLAGS = $(NGHTTP2_LIBS) $(CURL_LIBS) $(LIBXML2_LIBS) $(LIBESDKLOGAPI_LIBS) $(LIBOPENSSL_LIBS) $(LIBPCRE_LIBS) $(LIBSSH2_LIBS) $(LIBSECUREC_LIBS) $(LIBICONV_LIBS) $(PTHREAD)


# --------------------------------------------------------------------------
# Default targets are everything

.PHONY: all
all: exported


# --------------------------------------------------------------------------
# Exported targets are the library and driver program

.PHONY: exported
exported: libeSDKOBS headers


# --------------------------------------------------------------------------
# Install target

.PHONY: install
install: exported

# --------------------------------------------------------------------------
# Uninstall target

 .PHONY: uninstall
 uninstall:

# --------------------------------------------------------------------------
# Compile target patterns

$(BUILD)/obj/%.o: src/%.c
	$(QUIET_ECHO) $@: Compiling object
	@ mkdir -p $(dir $(BUILD)/dep/$<)
	@ $(CC) $(CFLAGS) -M -MG -MQ $@ -DCOMPILINGDEPENDENCIES \
        -o $(BUILD)/dep/$(<:%.c=%.d) -c $<
	@ mkdir -p $(dir $@)
	$(VERBOSE_SHOW) $(CC) $(CFLAGS) -fstack-protector 			\
									--param ssp-buffer-size=4	\
									-Wl,-z,relro,-z,now 		\
									-Wl,-z,noexecstack  		\
									-fPIC -std=c99		    	\
									-o $@ -c $<

$(BUILD)/obj/%.do: src/%.c
	$(QUIET_ECHO) $@: Compiling dynamic object
	@ mkdir -p $(dir $(BUILD)/dep/$<)
	@ $(CC) $(CFLAGS) -M -MG -MQ $@ -DCOMPILINGDEPENDENCIES \
        -o $(BUILD)/dep/$(<:%.c=%.dd) -c $<
	@ mkdir -p $(dir $@)
	$(VERBOSE_SHOW) $(CC) $(CFLAGS) -fstack-protector 			\
									--param ssp-buffer-size=4	\
									-Wl,-z,relro,-z,now 		\
									-Wl,-z,noexecstack  		\
									-fPIC -std=c99				\
									-o $@ -c $< 


# --------------------------------------------------------------------------
# libeSDKOBS library targets

ifdef BUILD_FOR_MACOS
LIBOBS_SHARED = $(BUILD)/lib/libeSDKOBS.dylib
else
LIBOBS_SHARED = $(BUILD)/lib/libeSDKOBS.so
endif
LIBOBS_STATIC = $(BUILD)/lib/libeSDKOBS.a

.PHONY: libeSDKOBS
libeSDKOBS: $(LIBOBS_SHARED) $(LIBOBS_STATIC)

LIBOBS_SOURCES := bucket.c  object.c request.c request_context.c \
                 response_headers_handler.c simplexml.c util.c log.c \
				 request_util.c general.c error_parser.c

$(LIBOBS_SHARED): $(LIBOBS_SOURCES:%.c=$(BUILD)/obj/%.do)
	$(QUIET_ECHO) $@: Building shared library
	@ mkdir -p $(dir $@)
ifdef BUILD_FOR_MACOS
    $(VERBOSE_SHOW) $(CC) -shared  \
						-fstack-protector 			\
						--param ssp-buffer-size=4	\
						-fPIC            			\
						-o $@ $^  $(LDFLAGS)
else
	$(VERBOSE_SHOW) $(CC) -shared -Wl,-soname,libeSDKOBS.so \
						-fstack-protector 			\
						--param ssp-buffer-size=4	\
						-Wl,-z,relro,-z,now 		\
						-Wl,-z,noexecstack   		\
						-fPIC            			\
						-o $@ $^  $(LDFLAGS)
endif

$(LIBOBS_STATIC): $(LIBOBS_SOURCES:%.c=$(BUILD)/obj/%.o)
	$(QUIET_ECHO) $@: Building static library
	@ mkdir -p $(dir $@)
	$(VERBOSE_SHOW) $(AR) cr $@ $^


# --------------------------------------------------------------------------
# Driver program targets

.PHONY: demo
demo: $(BUILD)/bin/demo

$(BUILD)/bin/demo: $(BUILD)/obj/demo.o $(LIBOBS_SHARED)
	$(QUIET_ECHO) $@: Building executable
	@ mkdir -p $(dir $@)
	$(VERBOSE_SHOW) $(CC) -fstack-protector 		\
						--param ssp-buffer-size=4	\
						-Wl,-z,relro,-z,now 		\
						-Wl,-z,noexecstack  		\
						-fPIC						\
						-o $@ $^ $(LDFLAGS)


# --------------------------------------------------------------------------
# libeSDKOBS header targets

.PHONY: headers
headers: $(BUILD)/include/eSDKOBS.h

$(BUILD)/include/eSDKOBS.h: inc/eSDKOBS.h
	$(QUIET_ECHO) $@: Linking header
	@ mkdir -p $(dir $@)
	cp $< $@


# --------------------------------------------------------------------------
# Test targets

.PHONY: test
test: $(BUILD)/bin/testsimplexml
$(BUILD)/bin/testsimplexml: $(BUILD)/obj/testsimplexml.o $(LIBOBS_STATIC)
	$(QUIET_ECHO) $@: Building executable
	@ mkdir -p $(dir $@)
	$(VERBOSE_SHOW) $(CC) -fstack-protector 		\
						--param ssp-buffer-size=4	\
						-Wl,-z,relro,-z,now 		\
						-Wl,-z,noexecstack  		\
						-fPIC						\
						-o $@ $^ $(LIBXML2_LIBS) $(LDFLAGS)


# --------------------------------------------------------------------------
# Clean target

.PHONY: clean
clean:
	$(QUIET_ECHO) $(BUILD): Cleaning
	$(VERBOSE_SHOW) rm -rf $(BUILD)

.PHONY: distclean
distclean:
	$(QUIET_ECHO) $(BUILD): Cleaning
	$(VERBOSE_SHOW) rm -rf $(BUILD)


# --------------------------------------------------------------------------
# Clean dependencies target

.PHONY: cleandeps
cleandeps:
	$(QUIET_ECHO) $(BUILD)/dep: Cleaning dependencies
	$(VERBOSE_SHOW) rm -rf $(BUILD)/dep


# --------------------------------------------------------------------------
# Dependencies

ALL_SOURCES := $(LIBOBS_SOURCES)

$(foreach i, $(ALL_SOURCES), $(eval -include $(BUILD)/dep/src/$(i:%.c=%.d)))
$(foreach i, $(ALL_SOURCES), $(eval -include $(BUILD)/dep/src/$(i:%.c=%.dd)))


# --------------------------------------------------------------------------
# Debian package target

DEBPKG = $(BUILD)/pkg/libOBS_$(LIBOBS_VER).deb
DEBDEVPKG = $(BUILD)/pkg/libOBS-dev_$(LIBOBS_VER).deb

.PHONY: deb
deb: $(DEBPKG) $(DEBDEVPKG)

$(DEBPKG): DEBARCH = $(shell dpkg-architecture | grep ^DEB_BUILD_ARCH= | \
                       cut -d '=' -f 2)
$(DEBPKG): exported $(BUILD)/deb/DEBIAN/control $(BUILD)/deb/DEBIAN/shlibs \
           $(BUILD)/deb/DEBIAN/postinst \
           $(BUILD)/deb/usr/share/doc/libOBS/changelog.gz \
           $(BUILD)/deb/usr/share/doc/libOBS/changelog.Debian.gz \
           $(BUILD)/deb/usr/share/doc/libOBS/copyright
	DESTDIR=$(BUILD)/deb/usr $(MAKE) install
	rm -rf $(BUILD)/deb/usr/include
	rm -f $(BUILD)/deb/usr/lib/libOBS.a
	@mkdir -p $(dir $@)
	fakeroot dpkg-deb -b $(BUILD)/deb $@
	mv $@ $(BUILD)/pkg/libOBS_$(LIBOBS_VER)_$(DEBARCH).deb

$(DEBDEVPKG): DEBARCH = $(shell dpkg-architecture | grep ^DEB_BUILD_ARCH= | \
                          cut -d '=' -f 2)
$(DEBDEVPKG): exported $(BUILD)/deb-dev/DEBIAN/control \
           $(BUILD)/deb-dev/usr/share/doc/libOBS-dev/changelog.gz \
           $(BUILD)/deb-dev/usr/share/doc/libOBS-dev/changelog.Debian.gz \
           $(BUILD)/deb-dev/usr/share/doc/libOBS-dev/copyright
	DESTDIR=$(BUILD)/deb-dev/usr $(MAKE) install
	rm -rf $(BUILD)/deb-dev/usr/bin
	rm -f $(BUILD)/deb-dev/usr/lib/libOBS.so*
	@mkdir -p $(dir $@)
	fakeroot dpkg-deb -b $(BUILD)/deb-dev $@
	mv $@ $(BUILD)/pkg/libOBS-dev_$(LIBOBS_VER)_$(DEBARCH).deb

$(BUILD)/deb/DEBIAN/control: debian/control
	@mkdir -p $(dir $@)
	echo -n "Depends: " > $@
	dpkg-shlibdeps -Sbuild -O $(BUILD)/lib/libOBS.so.$(LIBOBS_VER_MAJOR) | \
            cut -d '=' -f 2- >> $@
	sed -e 's/LIBOBS_VERSION/$(LIBOBS_VER)/' \
            < $< | sed -e 's/DEBIAN_ARCHITECTURE/$(DEBARCH)/' | \
            grep -v ^Source: >> $@

$(BUILD)/deb-dev/DEBIAN/control: debian/control.dev
	@mkdir -p $(dir $@)
	sed -e 's/LIBOBS_VERSION/$(LIBOBS_VER)/' \
            < $< | sed -e 's/DEBIAN_ARCHITECTURE/$(DEBARCH)/' > $@

$(BUILD)/deb/DEBIAN/shlibs:
	echo -n "libOBS $(LIBOBS_VER_MAJOR) libOBS " > $@
	echo "(>= $(LIBOBS_VER))" >> $@

$(BUILD)/deb/DEBIAN/postinst: debian/postinst
	@mkdir -p $(dir $@)
	cp $< $@

$(BUILD)/deb/usr/share/doc/libOBS/copyright: LICENSE
	@mkdir -p $(dir $@)
	cp $< $@
	@echo >> $@
	@echo -n "An alternate location for the GNU General Public " >> $@
	@echo "License version 3 on Debian" >> $@
	@echo "systems is /usr/share/common-licenses/GPL-3." >> $@

$(BUILD)/deb-dev/usr/share/doc/libOBS-dev/copyright: LICENSE
	@mkdir -p $(dir $@)
	cp $< $@
	@echo >> $@
	@echo -n "An alternate location for the GNU General Public " >> $@
	@echo "License version 3 on Debian" >> $@
	@echo "systems is /usr/share/common-licenses/GPL-3." >> $@

$(BUILD)/deb/usr/share/doc/libOBS/changelog.gz: debian/changelog
	@mkdir -p $(dir $@)
	gzip --best -c $< > $@

$(BUILD)/deb-dev/usr/share/doc/libOBS-dev/changelog.gz: debian/changelog
	@mkdir -p $(dir $@)
	gzip --best -c $< > $@

$(BUILD)/deb/usr/share/doc/libOBS/changelog.Debian.gz: debian/changelog.Debian
	@mkdir -p $(dir $@)
	gzip --best -c $< > $@

$(BUILD)/deb-dev/usr/share/doc/libOBS-dev/changelog.Debian.gz: \
    debian/changelog.Debian
	@mkdir -p $(dir $@)
	gzip --best -c $< > $@


