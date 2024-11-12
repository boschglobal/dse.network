# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

###############
## Docker Images.
DSE_CDOCGEN_IMAGE ?= dse-cdocgen:latest
DSE_PLANTUML_IMAGE ?= dse-plantuml:latest


###############
## Document Modules

# File "_index.md"
define _INDEX_MD_FILE =
---
title: "DSE Network API Reference"
linkTitle: "Network"
weight: 800
---

## DSE Network API Reference
endef
export _INDEX_MD_FILE
DOC_OUTPUT_index := doc/content/apis/network/_index.md

# Module "network"
DOC_INPUT_network := dse/network/network.h
DOC_CDIR_network := dse/network/network.c,dse/network/schedule.c,dse/network/parser.c,dse/network/loader.c,dse/network/engine.c,dse/network/encoder.c,
DOC_OUTPUT_network := doc/content/apis/network/network.md
DOC_LINKTITLE_network := Network
DOC_TITLE_network := "Network API Reference"


# Module "function"
DOC_INPUT_function := dse/network/examples/stub/functions/function.h
DOC_CDIR_function := dse/network/examples/stub/functions/counters.c,dse/network/examples/stub/functions/crc.c,dse/network/examples/stub/functions/function.c
DOC_OUTPUT_function := doc/content/apis/network/functions.md
DOC_LINKTITLE_function := Functions
DOC_TITLE_function := "Example Network Function API Reference"


# Targets
DOC_C_MODULES := network function


.PHONY: index
index:
	@cd ..; mkdir -p $$(dirname $(DOC_OUTPUT_index))
	@cd ..; echo "$$_INDEX_MD_FILE" > $(DOC_OUTPUT_index)

.PHONY: $(DOC_C_MODULES)
$(DOC_C_MODULES):
	@cd ..; mkdir -p $$(dirname $(DOC_OUTPUT_$@))
	@cd ..; docker run -it --rm \
		--volume $$(pwd):/tmp/repo \
		--workdir /tmp/repo \
		$(DSE_CDOCGEN_IMAGE) \
			--input $(DOC_INPUT_$@) \
			--output $(DOC_OUTPUT_$@) \
			--cdir $(DOC_CDIR_$@) \
			--title $(DOC_TITLE_$@) \
			--linktitle $(DOC_LINKTITLE_$@)
	@cd ../$$(dirname $(DOC_OUTPUT_$@)); docker run -it --rm \
		--volume $$(pwd):/tmp/repo \
		--workdir /tmp/repo \
		$(DSE_PLANTUML_IMAGE) \
			-tpng  $$(basename $(DOC_OUTPUT_$@))

.PHONY: generate
generate: index $(DOC_C_MODULES)
