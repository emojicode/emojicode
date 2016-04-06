VERSION = 0.2.0-beta.4

CC ?= gcc
CXX ?= g++

COMPILER_CFLAGS = -c -Wall -std=c++11 -Ofast -iquote . -iquote EmojicodeReal-TimeEngine/ -iquote EmojicodeCompiler/
COMPILER_LDFLAGS =

COMPILER_SRCDIR = EmojicodeCompiler
COMPILER_SOURCES = $(wildcard $(COMPILER_SRCDIR)/*.cpp)
COMPILER_OBJECTS = $(COMPILER_SOURCES:%.cpp=%.o)
COMPILER_BINARY = emojicodec

ENGINE_CFLAGS = -Ofast -iquote . -iquote EmojicodeReal-TimeEngine/ -iquote EmojicodeCompiler -std=gnu11 -Wall -Wno-unused-result $(if $(HEAP_SIZE),-DheapSize=$(HEAP_SIZE))
ENGINE_LDFLAGS = -lm -ldl -lpthread -rdynamic

ENGINE_SRCDIR = EmojicodeReal-TimeEngine
ENGINE_SOURCES = $(wildcard $(ENGINE_SRCDIR)/*.c)
ENGINE_OBJECTS = $(ENGINE_SOURCES:%.c=%.o)
ENGINE_BINARY = emojicode

PACKAGE_CFLAGS = -O3 -iquote . -std=c11 -Wno-unused-result -fPIC
PACKAGE_LDFLAGS = -shared -fPIC
ifeq ($(shell uname), Darwin)
PACKAGE_LDFLAGS += -undefined dynamic_lookup
endif

PACKAGES_DIR=DefaultPackages
PACKAGES=files SDL sqlite

DIST_NAME=Emojicode-$(VERSION)-$(shell $(CC) -dumpmachine)
DIST_BUILDS=builds
DIST=$(DIST_BUILDS)/$(DIST_NAME)

TESTS_DIR=tests

.PHONY: builds tests install dist

all: builds $(COMPILER_BINARY) $(ENGINE_BINARY) $(addsuffix .so,$(PACKAGES)) dist

$(COMPILER_BINARY): $(COMPILER_OBJECTS) EmojicodeReal-TimeEngine/utf8.o
	$(CXX) $^ -o $(DIST)/$(COMPILER_BINARY) $(COMPILER_LDFLAGS)

$(COMPILER_OBJECTS): %.o: %.cpp
	$(CXX) -c $< -o $@ $(COMPILER_CFLAGS)

$(ENGINE_BINARY): $(ENGINE_OBJECTS)
	$(CC) $^ -o $(DIST)/$(ENGINE_BINARY) $(ENGINE_LDFLAGS)

$(ENGINE_OBJECTS): %.o: %.c
	$(CC) -c $< -o $@ $(ENGINE_CFLAGS)

define package
PKG_$(1)_LDFLAGS = $$(PACKAGE_LDFLAGS)
ifeq ($(1), SDL)
PKG_$(1)_LDFLAGS += -lSDL2
endif
PKG_$(1)_SOURCES = $$(wildcard $$(PACKAGES_DIR)/$(1)/*.c)
PKG_$(1)_OBJECTS = $$(PKG_$(1)_SOURCES:%.c=%.o)
$(1).so: $$(PKG_$(1)_OBJECTS)
	$$(CC) $$(PKG_$(1)_LDFLAGS) $$^ -o $(DIST)/$$@ -iquote $$(<D)
$$(PKG_$(1)_OBJECTS): %.o: %.c
	$$(CC) $$(PACKAGE_CFLAGS) -c $$< -o $$@
endef

$(foreach pkg,$(PACKAGES),$(eval $(call package,$(pkg))))

clean:
	rm -f $(ENGINE_OBJECTS) $(COMPILER_OBJECTS) $(PACKAGES_DIR)/*/*.o

builds:
	mkdir -p $(DIST)

define testFile
$(DIST)/$(COMPILER_BINARY) -o $(TESTS_DIR)/$(1).emojib $(TESTS_DIR)/$(1).emojic
$(DIST)/$(ENGINE_BINARY) $(TESTS_DIR)/$(1).emojib
endef

install: dist
	cd $(DIST) && ./install.sh

tests:
	$(call testFile,stringTest)
	$(call testFile,primitiveMethodsTest)
	$(call testFile,listTest)
	$(call testFile,dictionaryTest)
	$(call testFile,rangeTest)
	$(call testFile,dataTest)
	$(call testFile,mathTest)
	$(call testFile,fileTest)
	$(call testFile,systemTest)
	$(call testFile,jsonTest)

dist:
	rm -f $(DIST)/install.sh
	rm -rf $(DIST)/headers
	cp install.sh $(DIST)/install.sh
	cp -r headers/ $(DIST)/headers
	tar -czf $(DIST).tar.gz -C $(DIST_BUILDS) $(DIST_NAME)
