// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

package main

import (
	"flag"
	"log/slog"
	"os"

	"github.com/boschglobal/dse.network/extra/tools/network/internal/app/generate"
	"github.com/boschglobal/dse.network/extra/tools/network/internal/pkg/patch"
)

var (
	cmds = []CommandRunner{
		NewHelpCommand("help"),
		generate.NewGenNetworkCommand("gen-network"),
		generate.NewGenSignalGroupCommand("gen-signalgroup"),
		patch.NewPatchNetworkCommand("patch-network"),
	}
)

func main() {
	flag.Usage = PrintUsage
	if len(os.Args) == 1 {
		PrintUsage()
		os.Exit(1)
	}
	if err := DispatchCommand(os.Args[1]); err != nil {
		slog.Info("Command error: %v", err)
		os.Exit(2)
	}
}
