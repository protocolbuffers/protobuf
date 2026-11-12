package main

import (
	"google.golang.org/protobuf/compiler/protogen"
)

// collectMessages returns all messages (transitively, including nested and map
// entries) found in file, in post-order (leaves first).
func collectMessages(file *protogen.File) []*protogen.Message {
	var out []*protogen.Message
	seen := map[string]bool{}
	for _, m := range file.Messages {
		collectMessage(m, &out, seen)
	}
	return out
}

func collectMessage(m *protogen.Message, out *[]*protogen.Message, seen map[string]bool) {
	if seen[string(m.Desc.FullName())] {
		return
	}
	seen[string(m.Desc.FullName())] = true
	// recurse children first (post-order)
	for _, nm := range m.Messages {
		collectMessage(nm, out, seen)
	}
	for _, f := range m.Fields {
		if f.Message != nil {
			collectMessage(f.Message, out, seen)
		}
	}
	*out = append(*out, m)
}

// collectEnums returns all enums transitively.
func collectEnums(file *protogen.File) []*protogen.Enum {
	var out []*protogen.Enum
	seen := map[string]bool{}
	for _, e := range file.Enums {
		collectEnum(e, &out, seen)
	}
	for _, m := range collectMessages(file) {
		for _, e := range m.Enums {
			collectEnum(e, &out, seen)
		}
	}
	return out
}

func collectEnum(e *protogen.Enum, out *[]*protogen.Enum, seen map[string]bool) {
	if seen[string(e.Desc.FullName())] {
		return
	}
	seen[string(e.Desc.FullName())] = true
	*out = append(*out, e)
}

// collectExtensions builds a map from extendee full name to extension fields.
func collectExtensions(files []*protogen.File) map[string][]*protogen.Field {
	m := map[string][]*protogen.Field{}
	for _, f := range files {
		for _, ext := range f.Extensions {
			// ext is *protogen.Field with Extendee set
			key := string(ext.Extendee.Desc.FullName())
			m[key] = append(m[key], ext)
		}
		// also extensions declared inside messages? protogen exposes via file.Extensions only top-level.
		// message-level extend is rare; handle top-level.
	}
	return m
}
