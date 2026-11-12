package main

import (
	"flag"
	"strings"

	"google.golang.org/protobuf/compiler/protogen"
)

func main() {
	opts, run := pluginOptions()
	opts.Run(run)
}

// pluginOptions returns the protogen options (with plugin parameter parsing)
// and the generation callback. Split out from main so tests can drive the
// plugin in-process with a hand-built CodeGeneratorRequest.
func pluginOptions() (protogen.Options, func(*protogen.Plugin) error) {
	var flags flag.FlagSet
	encode := flags.Bool("encode", true, "generate encode functions")
	opts := protogen.Options{ParamFunc: flags.Set}
	return opts, func(gen *protogen.Plugin) error {
		gen.SupportedFeatures = 1 | 2
		gen.SupportedEditionsMinimum = 998  // EDITION_PROTO2
		gen.SupportedEditionsMaximum = 1002 // EDITION_2026
		// first pass: collect extensions globally
		exts := collectExtensions(gen.Files)

		for _, f := range gen.Files {
			if !f.Generate {
				continue
			}
			generateFile(gen, f, exts, *encode)
		}
		return nil
	}
}

func generateFile(gen *protogen.Plugin, file *protogen.File, exts map[string][]*protogen.Field, encode bool) {
	base := file.GeneratedFilenamePrefix
	if idx := strings.LastIndex(base, "/"); idx >= 0 {
		base = base[idx+1:]
	}
	out := gen.NewGeneratedFile(base+".sql", file.GoImportPath)
	gc := &genContext{out: out, file: file, exts: exts, generated: map[string]bool{}, encode: encode}
	gc.generate()
}
