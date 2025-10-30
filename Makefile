# Copyright 2023 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0


################
## DSE Projects.
export PACKAGE_ARCH ?= linux-amd64

DSE_CLIB_REPO ?= https://github.com/boschglobal/dse.clib
DSE_CLIB_VERSION ?= 1.0.41
export DSE_CLIB_URL ?= $(DSE_CLIB_REPO)/archive/refs/tags/v$(DSE_CLIB_VERSION).zip

DSE_MODELC_REPO ?= https://github.com/boschglobal/dse.modelc
DSE_MODELC_VERSION ?= 2.2.15
export DSE_MODELC_URL ?= $(DSE_MODELC_REPO)/archive/refs/tags/v$(DSE_MODELC_VERSION).zip
export DSE_MODELC_LIB_URL ?= $(DSE_MODELC_REPO)/releases/download/v$(DSE_MODELC_VERSION)/ModelC-$(DSE_MODELC_VERSION)-$(PACKAGE_ARCH).zip


###############
## Docker Images.
GCC_BUILDER_IMAGE ?= ghcr.io/boschglobal/dse-gcc-builder:latest
TESTSCRIPT_IMAGE ?= ghcr.io/boschglobal/dse-testscript:latest
SIMER_IMAGE ?= ghcr.io/boschglobal/dse-simer:$(DSE_MODELC_VERSION)
DSE_CLANG_FORMAT_IMAGE ?= ghcr.io/boschglobal/dse-clang-format:latest


###############
## Tools (Container Images).
TOOL_DIRS = network


###############
## Build parameters.
export NAMESPACE = dse
export MODULE = network
export EXTERNAL_BUILD_DIR ?= /tmp/$(NAMESPACE).$(MODULE)
export PACKAGE_ARCH_LIST ?= $(PACKAGE_ARCH)
export CMAKE_TOOLCHAIN_FILE ?= $(shell pwd -P)/extra/cmake/$(PACKAGE_ARCH).cmake
export MODELC_SANDBOX_DIR ?= $(shell pwd -P)/$(NAMESPACE)/$(MODULE)/build/_deps/dse_modelc-src
SUBDIRS = $(NAMESPACE)/$(MODULE)


###############
## Package parameters.
export PACKAGE_VERSION ?= 0.0.2
DIST_DIR := $(shell pwd -P)/$(NAMESPACE)/$(MODULE)/build/_dist
OSS_DIR = $(NAMESPACE)/__oss__
PACKAGE_DOC_NAME = DSE Network Model
PACKAGE_NAME = Network
PACKAGE_NAME_LC = network
PACKAGE_PATH = $(NAMESPACE)/dist


###############
## Test Parameters.
export HOST_DOCKER_WORKSPACE ?= $(shell pwd -P)
export TESTSCRIPT_E2E_DIR ?= tests/testscript/e2e
TESTSCRIPT_E2E_FILES = $(wildcard $(TESTSCRIPT_E2E_DIR)/*.txtar)
NETWORK_IMAGE ?= $(NAMESPACE)-$(MODULE)
NETWORK_TAG ?= test



ifneq ($(CI), true)
DOCKER_BUILDER_CMD := \
	mkdir -p $(EXTERNAL_BUILD_DIR); \
	docker run -it --rm \
		--user $$(id -u):$$(id -g) \
		--env CMAKE_TOOLCHAIN_FILE=/tmp/repo/extra/cmake/$(PACKAGE_ARCH).cmake \
		--env EXTERNAL_BUILD_DIR=$(EXTERNAL_BUILD_DIR) \
		--env GDB_CMD="$(GDB_CMD)" \
		--env PACKAGE_ARCH=$(PACKAGE_ARCH) \
		--env PACKAGE_VERSION=$(PACKAGE_VERSION) \
		--env MAKE_NPROC=$(MAKE_NPROC) \
		--volume $$(pwd):/tmp/repo \
		--volume $(EXTERNAL_BUILD_DIR):$(EXTERNAL_BUILD_DIR) \
		--workdir /tmp/repo \
		$(GCC_BUILDER_IMAGE)
endif

DSE_CLANG_FORMAT_CMD := docker run -it --rm \
	--user $$(id -u):$$(id -g) \
	--volume $$(pwd):/tmp/code \
	${DSE_CLANG_FORMAT_IMAGE}

default: build

.PHONY: build
build:
	@${DOCKER_BUILDER_CMD} $(MAKE) do-build

.PHONY: package
package:
	@${DOCKER_BUILDER_CMD} $(MAKE) do-package

.PHONY: tools
tools:
	for d in $(TOOL_DIRS) ;\
	do \
		mkdir -p extra/tools/$$d/build/stage ;\
		cp -r licenses -t extra/tools/$$d/build/stage ;\
		docker build -f extra/tools/$$d/build/package/Dockerfile \
				--build-arg="CCACHE_SECONDARY_STORAGE=$(CCACHE_SECONDARY_STORAGE)" \
				--tag $$d:test extra/tools/$$d ;\
	done;

do-test_cmocka-build:
	$(MAKE) -C tests/cmocka build

do-test_cmocka-run:
	$(MAKE) -C tests/cmocka run

test_cmocka:
ifeq ($(PACKAGE_ARCH), linux-amd64)
	@${DOCKER_BUILDER_CMD} $(MAKE) do-test_cmocka-build
	@${DOCKER_BUILDER_CMD} $(MAKE) do-test_cmocka-run
endif

.PHONY: test_e2e
test_e2e: do-test_testscript-e2e

.PHONY: test
test: test_cmocka test_e2e

do-test_testscript-e2e:
# Test debug; add '-v' to Testscript command (e.g. $(TESTSCRIPT_IMAGE) -v \).
ifeq ($(PACKAGE_ARCH), linux-amd64)
	@set -eu; for t in $(TESTSCRIPT_E2E_FILES) ;\
	do \
		echo "Running E2E Test: $$t" ;\
		docker run -i --rm \
			-e ENTRYDIR=$(HOST_DOCKER_WORKSPACE) \
			-v /var/run/docker.sock:/var/run/docker.sock \
			-v $(HOST_DOCKER_WORKSPACE):/repo \
			$(TESTSCRIPT_IMAGE) \
				-e ENTRYDIR=$(HOST_DOCKER_WORKSPACE) \
				-e NETWORK_IMAGE=$(NETWORK_IMAGE) \
				-e NETWORK_TAG=$(NETWORK_TAG) \
				-e SIMER=$(SIMER_IMAGE) \
				$$t ;\
	done;
endif


.PHONY: clean
clean:
	@${DOCKER_BUILDER_CMD} $(MAKE) do-clean
	for d in $(TOOL_DIRS) ;\
	do \
		rm -rf extra/tools/$$d/build/stage ;\
	done;
	docker images -qf dangling=true | xargs -r docker rmi

.PHONY: cleanall
cleanall:
	@${DOCKER_BUILDER_CMD} $(MAKE) do-cleanall
	docker ps --filter status=dead --filter status=exited -aq | xargs -r docker rm -v
	docker images -qf dangling=true | xargs -r docker rmi
	docker volume ls -qf dangling=true | xargs -r docker volume rm

.PHONY: oss
oss:
	@${DOCKER_BUILDER_CMD} $(MAKE) do-oss

.PHONY: generate
generate:
	$(MAKE) -C doc generate

.PHONY: format
format:
	@${DSE_CLANG_FORMAT_CMD} dse
	@${DSE_CLANG_FORMAT_CMD} tests/cmocka/

.PHONY: do-build
do-build:
	@for d in $(SUBDIRS); do ($(MAKE) -C $$d build ); done

.PHONY: do-package
do-package:
	@for d in $(SUBDIRS); do ($(MAKE) -C $$d package ); done

.PHONY: do-clean
do-clean:
	@for d in $(SUBDIRS); do ($(MAKE) -C $$d clean ); done
	$(MAKE) -C tests/cmocka clean
	rm -rf $(OSS_DIR)
	rm -rf doc_
	rm -rvf *.zip
	rm -rvf *.log

.PHONY: do-cleanall
do-cleanall: do-clean
	@for d in $(SUBDIRS); do ($(MAKE) -C $$d cleanall ); done
	$(MAKE) -C tests/cmocka cleanall

.PHONY: do-oss
do-oss:
	$(MAKE) -C extra/external oss

.PHONY: super-linter
super-linter:
	docker run --rm --volume $$(pwd):/tmp/lint \
		--env RUN_LOCAL=true \
		--env DEFAULT_BRANCH=main \
		--env IGNORE_GITIGNORED_FILES=true \
		--env FILTER_REGEX_EXCLUDE="(doc/content/.*|dse/network/examples/stub/.*|extra/tools/network/vendor/.*)" \
		--env VALIDATE_CPP=true \
		--env VALIDATE_DOCKERFILE=true \
		--env VALIDATE_MARKDOWN=true \
		--env VALIDATE_PYTHON_PYLINT=true \
		--env VALIDATE_PYTHON_FLAKE8=true \
		--env VALIDATE_YAML=true \
		ghcr.io/super-linter/super-linter:slim-v8
#		--env VALIDATE_GO=true \
