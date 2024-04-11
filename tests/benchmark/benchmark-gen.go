package main

import (
	"flag"
	"fmt"
	"os"
	"text/template"
)

func main() {
	var signalCount = flag.Int("signals", 1, "benchmark N signals")
	var netCt = flag.String("net_ct", "network_ct", "generated network (on basis cantools)")
	flag.Parse()

	fmt.Printf("Generate benchmark code for %d signals ...\n", *signalCount)
	file, _ := os.Create(fmt.Sprintf("build/%s.c", *netCt))
	defer file.Close()

	// Front matter
	tmpl := template.Must(template.ParseFiles(fmt.Sprintf("template/%s_front.tmpl", *netCt)))
	tmpl.Execute(file, nil)

	// Body content
	tmpl = template.Must(template.ParseFiles(fmt.Sprintf("template/%s_body.tmpl", *netCt)))
	for i := 0; i < *signalCount; i++ {
		tmpl.Execute(file, i)
	}
}
