# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

###############
## Docker Images.
DSE_CDOCGEN_IMAGE ?= ghcr.io/boschglobal/dse-cdocgen:latest
DSE_PLANTUML_IMAGE ?= ghcr.io/boschglobal/dse-plantuml:latest


###############
## Document Modules

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
generate: $(DOC_C_MODULES)
