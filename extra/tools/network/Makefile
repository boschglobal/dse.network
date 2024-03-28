# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0


SRC           := $(shell find . -type f -name '*.go' -print)
BIN_DIR       := $(CURDIR)/bin
CLI_TOOLS     ?= $(BIN_DIR)/network
GOFLAGS       :=
GOPRIVATE     ?= github.com/boschglobal

# Additional no-Go related settings.
CANTOOLS_TGZ    := $(shell pwd)/third_party/cantools/cantools.tar.gz
CANTOOLS_DIR    := $(shell mktemp -d)
CANTOOLS_REPO   = https://github.boschdevcloud.com/fsil-oss-mirrors/cantools.git
CANTOOLS_BRANCH = FSIL


.PHONY: all
all: build

.PHONY: build
build: cmd

cmd: $(CLI_TOOLS)

$(CLI_TOOLS): $(SRC)
	go build $(GOFLAGS) -o $@ ./cmd/$(subst $(BIN_DIR)/,,$@)


.PHONY: test
test:
	go test ./...
	go test ./... -tags e2e


.PHONY: lint
lint:
	go fmt -x ./...
	PATH=~/go/bin:$$PATH; goimports -w -v .
	docker run -t --rm \
		-v $$(pwd):/app \
		-v ~/.cache/golangci-lint/v1.54.2:/root/.cache \
		-w /app \
		golangci/golangci-lint:latest \
		golangci-lint run -v ./...


.PHONY: clean
clean:
	@rm -rf '$(BIN_DIR)'


.PHONY: update
update: update_cantools

update_cantools:
	cd $(CANTOOLS_DIR); git clone --depth=1 -b $(CANTOOLS_BRANCH) $(CANTOOLS_REPO)
	cd $(CANTOOLS_DIR); rm -r cantools/examples cantools/tests cantools/docs
	cd $(CANTOOLS_DIR)/cantools; rm -rf .git; git init; git add .; git commit -m"fsil cantools"
	cd $(CANTOOLS_DIR); tar -cvzf $(CANTOOLS_TGZ) cantools
	rm -rf $(CANTOOLS_DIR)
