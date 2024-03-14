# Copyright 2023 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0


PACKAGE       := $(shell basename $(shell pwd))
SRC           := $(shell find . -type f -name '*.go' -print)
BIN_DIR       := $(CURDIR)/bin
CLI_TOOLS     ?= $(BIN_DIR)/$(PACKAGE)-parser
GOFLAGS       :=
DOCKER_DIRS   := build/parser
DOCKER_PREFIX ?= fsil


.PHONY: all
all: build


.PHONY: build
build: cmd

cmd: $(CLI_TOOLS)

$(CLI_TOOLS): $(SRC)
	go build $(GOFLAGS) -o $@ ./cmd/$(subst $(BIN_DIR)/$(PACKAGE)-,,$@)

.PHONY: docker
docker: build
	for d in $(DOCKER_DIRS) ;\
	do \
		docker build -f $$d/Dockerfile \
			--tag $(DOCKER_PREFIX)-$(PACKAGE)-$$(basename $$d):latest . ;\
	done;


.PHONY: clean
clean:
	@rm -rf '$(BIN_DIR)'
	for d in $(DOCKER_DIRS) ;\
	do \
		docker images -q $(DOCKER_PREFIX)-$(PACKAGE)-$$(basename $$d) | xargs -r docker rmi -f ;\
		docker images -q */*/$(DOCKER_PREFIX)-$(PACKAGE)-$$(basename $$d) | xargs -r docker rmi -f ;\
	done;
	docker images -qf dangling=true | xargs -r docker rmi