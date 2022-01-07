
<!---
This document contains embedded graphviz diagrams inside ```dot blocks.

To convert it to rendered form using render.py:
  $ ./render.py wrapping-upb.in.md

You can also live-preview this document with all diagrams using Markdown Preview Enhanced
in Visual Studio Code:
  https://marketplace.visualstudio.com/items?itemName=shd101wyy.markdown-preview-enhanced
--->

# Wrapping upb in other languages

upb is a C kernel that is designed to be wrapped in other languages.  This is a
guide for creating a new protobuf implementation based on upb.

## What you will need

There are certain things that the language runtime must provide in order to be
wrapped by upb.

1. **Finalizers, Destructors, or Cleaners**: This is one unavoidable
   requirement: the language *must* provide finalizers or destructors of some sort.
   There must be a way of calling a C function when the language GCs or otherwise
   destroys an object.  We don't care much whether it is a finalizer, a destructor,
   or a cleaner, as long as it gets called eventually when the object is destroyed.
   Without finalizers, we would have no way of cleaning up upb data and everything
   would leak.
2. **HashMap with weak values**: This is not an absolute requirement, but in
   languages with automatic memory management, we generally end up wanting a
   hash map with weak values to act as a `upb_msg* -> wrapper` object cache.
   We want the values to be weak (not the keys).

## Reflection vs. Direct Access

Each language wrapping upb gets to decide whether it will access messages
through *reflection* or through *direct access*.  This decision has some deep
implications that will affect the design, features, and performance of your
library.

### Reflection

The simplest option is to load full reflection data into the upb library at
runtime.  You can load reflection data using serialized descriptors, which are a
stable and widely supported format across all protobuf tooling.

```c
  // A upb_symtab is a dynamic container that we can load reflection data into.
  upb_symtab* symtab = upb_symtab_new();

  // We load reflection data via a serialized descriptor.  The code generator
  // for your language should embed serialized descriptors into your generated
  // files; for each file you load, you can add the descriptor to the symtab as
  // shown.
  upb_arena *tmp = upb_arena_new();
  google_protobuf_FileDescriptorProto* file =
      google_protobuf_FileDescriptorProto_parse(desc_data, desc_size, tmp);
  if (!file || !upb_symtab_addfile(symtab, file, NULL)) {
    // Handle error.
  }
  upb_arena_free(tmp);

  // At application exit, we free the symtab.
  upb_symtab_free(symtab);
```

The `upb_symtab` will give you full access to all data from the `.proto` file,
including convenient APIs like looking up a field by name. It will allow you to
use JSON and text format.  The APIs for accessing a message through reflection
are simple and well-supported.  These APIs cleanly encapsulate upb's internal
implementation details.  

```c
  upb_symtab* symtab = BuildSymtab();

  // Look up a message type in the symtab.
  const upb_msgdef* m = upb_symtab_lookupmsg(symtab, "FooMessage");

  // Construct a new message of this type, via reflection.
  upb_arena *arena = upb_arena_new();
  upb_msg *msg = upb_msg_new(m, arena);

  // Set a message field using reflection.
  const upb_fielddef* f = upb_msgdef_ntof("bar_field");
  upb_msgval val = {.int32_val = 123};
  upb_msg_set(m, f, val, arena);

  // Free the message and symtab.
  upb_arena_free(arena);
  upb_symtab_free(symtab);
```

Using reflection is an natural choice in heavily reflective, dynamic runtimes
like Python, Ruby, PHP, or Lua.  These languages generally perform method
dispatch through a dictionary/hash table anyway, so we are not adding any extra
overhead by using upb's hash table to lookup fields by name at field access
time.

### Direct Access

Using reflection has some downsides.  Reflection data is relatively large, both
in your binary (at rest) and in RAM (at runtime).  It contains names of
everything, and these names will be exposed in your binary.  Reflection APIs for
accessing a message will have more overhead than you might want, especially if
crossing the FFI boundary for your language runtime imposes significant
overhead.

We can reduce these overheads by using *direct access*.  upb's parser and
serializer do not actually require full reflection data, they use a more compact
data structure known as **mini tables**.  Mini tables will take up less space
than reflection, both in the binary and in RAM, and they will not leak field
names.  Mini tables will let us parse and serialize binary wire format data
without reflection.

```c
  // TODO: demonstrate upb API for loading mini table data at runtime.
  // This API does not exist yet.
```

To access messages themselves without the reflection API, we will be using
different, lower-level APIs that will require you to supply precise data such as
the offset of a given field.  This is information that will come from the upb
compiler framework, and the correctness (and even memory safety!) of the program
will rely on you passing these values through from the upb compiler libraries to
the upb runtime correctly.

```c
  // TODO: demonstrate using low-level APIs for direct field access.
  // These APIs do not exist yet.
```

It can even be possible in certain circumstances to bypass the upb API completely
and access raw field data directly at a given offset, using unsafe APIs like
`sun.misc.unsafe`.  This can theoretically allow for field access that is no
more expensive than referencing a struct/class field.

```java
import sun.misc.Unsafe;

class FooProto {
  private final long addr;
  private final Arena arena;

  // Accessor that a Java library built on upb could conceivably generate.
  long getFoo() {
    // The offset 1234 came from the upb compiler library, and was injected by the
    // Java+upb code generator.
    return Unsafe.getLong(self.addr + 1234);
  }
}
```

It is always possible for a library built on direct access to also load reflection
data as an add-on, optional package for when users want JSON, text format, or
reflection-based access to a message.  You do not give up any of these possibilities
by using direct access.

However, using direct access does have some noticeable downsides.  It requires
tighter coupling with upb's implementation details, as the mini table format is
upb-specific and requires building your code generator against upb's compiler
libraries.  Any direct access of memory is especially tightly coupled, and would
need to be changed if upb's in-memory format ever changes.  It also is more
prone to hard-to-debug memory errors if you make any mistakes.

## Memory Management

One of the core design challenges when wrapping upb is memory management.  Every
language runtime will have some memory management system, whether it is
garbage collection, reference counting, manual memory management, or some hybrid
of these.  upb is written in C and uses arenas for memory management, but upb is
designed to integrate with a wide variety of memory management schemes, and it
provides a number of tools for making this integration as smooth as possible.

### Arenas

upb defines data structures in C to represent messages, arrays (repeated
fields), and maps.  A protobuf message is a hierarchical tree of these objects.
For example, a relatively simple protobuf tree might look something like this:

<div align=center><?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"
 "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<!-- Generated by graphviz version 2.43.0 (0)
 -->
<!-- Title: G Pages: 1 -->
<svg width="222pt" height="98pt"
 viewBox="0.00 0.00 222.00 98.00" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<g id="graph0" class="graph" transform="scale(1 1) rotate(0) translate(4 94)">
<title>G</title>
<!-- upb_msg -->
<g id="node1" class="node">
<title>upb_msg</title>
<path fill="#7fc97f" stroke="black" d="M77,-63C77,-63 12,-63 12,-63 6,-63 0,-57 0,-51 0,-51 0,-39 0,-39 0,-33 6,-27 12,-27 12,-27 77,-27 77,-27 83,-27 89,-33 89,-39 89,-39 89,-51 89,-51 89,-57 83,-63 77,-63"/>
<text text-anchor="middle" x="44.5" y="-41.3" font-family="Times,serif" font-size="14.00">upb Message</text>
</g>
<!-- upb_msg2 -->
<g id="node2" class="node">
<title>upb_msg2</title>
<path fill="#7fc97f" stroke="black" d="M202,-90C202,-90 137,-90 137,-90 131,-90 125,-84 125,-78 125,-78 125,-66 125,-66 125,-60 131,-54 137,-54 137,-54 202,-54 202,-54 208,-54 214,-60 214,-66 214,-66 214,-78 214,-78 214,-84 208,-90 202,-90"/>
<text text-anchor="middle" x="169.5" y="-68.3" font-family="Times,serif" font-size="14.00">upb Message</text>
</g>
<!-- upb_msg&#45;&gt;upb_msg2 -->
<g id="edge1" class="edge">
<title>upb_msg&#45;&gt;upb_msg2</title>
<path fill="none" stroke="black" d="M89.21,-54.6C97.55,-56.43 106.36,-58.36 114.97,-60.25"/>
<polygon fill="black" stroke="black" points="114.41,-63.71 124.93,-62.44 115.91,-56.87 114.41,-63.71"/>
</g>
<!-- upb_array -->
<g id="node3" class="node">
<title>upb_array</title>
<path fill="#7fc97f" stroke="black" d="M193,-36C193,-36 146,-36 146,-36 140,-36 134,-30 134,-24 134,-24 134,-12 134,-12 134,-6 140,0 146,0 146,0 193,0 193,0 199,0 205,-6 205,-12 205,-12 205,-24 205,-24 205,-30 199,-36 193,-36"/>
<text text-anchor="middle" x="169.5" y="-14.3" font-family="Times,serif" font-size="14.00">upb Array</text>
</g>
<!-- upb_msg&#45;&gt;upb_array -->
<g id="edge2" class="edge">
<title>upb_msg&#45;&gt;upb_array</title>
<path fill="none" stroke="black" d="M89.21,-35.4C100.42,-32.94 112.49,-30.3 123.75,-27.82"/>
<polygon fill="black" stroke="black" points="124.8,-31.18 133.81,-25.61 123.3,-24.34 124.8,-31.18"/>
</g>
</g>
</svg>
</div>
All upb objects are allocated from an arena.  An arena lets you allocate objects
individually, but you cannot free individual objects; you can only free the arena
as a whole.  When the arena is freed, all of the individual objects allocated
from that arena are freed together.

<div align=center><?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"
 "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<!-- Generated by graphviz version 2.43.0 (0)
 -->
<!-- Title: G Pages: 1 -->
<svg width="254pt" height="153pt"
 viewBox="0.00 0.00 254.00 153.00" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<g id="graph0" class="graph" transform="scale(1 1) rotate(0) translate(4 149)">
<title>G</title>
<g id="clust1" class="cluster">
<title>cluster_0</title>
<path fill="gray" stroke="black" d="M20,-8C20,-8 226,-8 226,-8 232,-8 238,-14 238,-20 238,-20 238,-125 238,-125 238,-131 232,-137 226,-137 226,-137 20,-137 20,-137 14,-137 8,-131 8,-125 8,-125 8,-20 8,-20 8,-14 14,-8 20,-8"/>
<text text-anchor="middle" x="123" y="-121.8" font-family="Times,serif" font-size="14.00">upb Arena</text>
</g>
<!-- upb_msg -->
<g id="node1" class="node">
<title>upb_msg</title>
<path fill="#7fc97f" stroke="black" d="M93,-79C93,-79 28,-79 28,-79 22,-79 16,-73 16,-67 16,-67 16,-55 16,-55 16,-49 22,-43 28,-43 28,-43 93,-43 93,-43 99,-43 105,-49 105,-55 105,-55 105,-67 105,-67 105,-73 99,-79 93,-79"/>
<text text-anchor="middle" x="60.5" y="-57.3" font-family="Times,serif" font-size="14.00">upb Message</text>
</g>
<!-- upb_array -->
<g id="node2" class="node">
<title>upb_array</title>
<path fill="#7fc97f" stroke="black" d="M209,-52C209,-52 162,-52 162,-52 156,-52 150,-46 150,-40 150,-40 150,-28 150,-28 150,-22 156,-16 162,-16 162,-16 209,-16 209,-16 215,-16 221,-22 221,-28 221,-28 221,-40 221,-40 221,-46 215,-52 209,-52"/>
<text text-anchor="middle" x="185.5" y="-30.3" font-family="Times,serif" font-size="14.00">upb Array</text>
</g>
<!-- upb_msg&#45;&gt;upb_array -->
<g id="edge1" class="edge">
<title>upb_msg&#45;&gt;upb_array</title>
<path fill="none" stroke="black" d="M105.21,-51.4C116.42,-48.94 128.49,-46.3 139.75,-43.82"/>
<polygon fill="black" stroke="black" points="140.8,-47.18 149.81,-41.61 139.3,-40.34 140.8,-47.18"/>
</g>
<!-- upb_msg2 -->
<g id="node3" class="node">
<title>upb_msg2</title>
<path fill="#7fc97f" stroke="black" d="M218,-106C218,-106 153,-106 153,-106 147,-106 141,-100 141,-94 141,-94 141,-82 141,-82 141,-76 147,-70 153,-70 153,-70 218,-70 218,-70 224,-70 230,-76 230,-82 230,-82 230,-94 230,-94 230,-100 224,-106 218,-106"/>
<text text-anchor="middle" x="185.5" y="-84.3" font-family="Times,serif" font-size="14.00">upb Message</text>
</g>
<!-- upb_msg&#45;&gt;upb_msg2 -->
<g id="edge2" class="edge">
<title>upb_msg&#45;&gt;upb_msg2</title>
<path fill="none" stroke="black" d="M105.21,-70.6C113.55,-72.43 122.36,-74.36 130.97,-76.25"/>
<polygon fill="black" stroke="black" points="130.41,-79.71 140.93,-78.44 131.91,-72.87 130.41,-79.71"/>
</g>
</g>
</svg>
</div>
In simple cases, the entire tree of objects will all live in a single arena.
This has the nice property that there cannot be any dangling pointers between
objects, since all objects are freed at the same time.

However upb allows you to create links between any two objects, whether or
not they are in the same arena.  The library does not know or care what arenas
the objects are in when you create links between them.

<div align=center><?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"
 "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<!-- Generated by graphviz version 2.43.0 (0)
 -->
<!-- Title: G Pages: 1 -->
<svg width="409pt" height="153pt"
 viewBox="0.00 0.00 409.00 153.00" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<g id="graph0" class="graph" transform="scale(1 1) rotate(0) translate(4 149)">
<title>G</title>
<g id="clust1" class="cluster">
<title>cluster_0</title>
<path fill="gray" stroke="black" d="M20,-8C20,-8 246,-8 246,-8 252,-8 258,-14 258,-20 258,-20 258,-125 258,-125 258,-131 252,-137 246,-137 246,-137 20,-137 20,-137 14,-137 8,-131 8,-125 8,-125 8,-20 8,-20 8,-14 14,-8 20,-8"/>
<text text-anchor="middle" x="133" y="-121.8" font-family="Times,serif" font-size="14.00">upb Arena 1</text>
</g>
<g id="clust2" class="cluster">
<title>cluster_1</title>
<path fill="gray" stroke="black" d="M290,-62C290,-62 381,-62 381,-62 387,-62 393,-68 393,-74 393,-74 393,-125 393,-125 393,-131 387,-137 381,-137 381,-137 290,-137 290,-137 284,-137 278,-131 278,-125 278,-125 278,-74 278,-74 278,-68 284,-62 290,-62"/>
<text text-anchor="middle" x="335.5" y="-121.8" font-family="Times,serif" font-size="14.00">upb Arena 2</text>
</g>
<!-- upb_msg -->
<g id="node1" class="node">
<title>upb_msg</title>
<path fill="#7fc97f" stroke="black" d="M103,-79C103,-79 28,-79 28,-79 22,-79 16,-73 16,-67 16,-67 16,-55 16,-55 16,-49 22,-43 28,-43 28,-43 103,-43 103,-43 109,-43 115,-49 115,-55 115,-55 115,-67 115,-67 115,-73 109,-79 103,-79"/>
<text text-anchor="middle" x="65.5" y="-57.3" font-family="Times,serif" font-size="14.00">upb Message 1</text>
</g>
<!-- upb_array -->
<g id="node2" class="node">
<title>upb_array</title>
<path fill="#7fc97f" stroke="black" d="M224,-52C224,-52 177,-52 177,-52 171,-52 165,-46 165,-40 165,-40 165,-28 165,-28 165,-22 171,-16 177,-16 177,-16 224,-16 224,-16 230,-16 236,-22 236,-28 236,-28 236,-40 236,-40 236,-46 230,-52 224,-52"/>
<text text-anchor="middle" x="200.5" y="-30.3" font-family="Times,serif" font-size="14.00">upb Array</text>
</g>
<!-- upb_msg&#45;&gt;upb_array -->
<g id="edge1" class="edge">
<title>upb_msg&#45;&gt;upb_array</title>
<path fill="none" stroke="black" d="M115.27,-51.1C128.26,-48.46 142.22,-45.63 154.96,-43.04"/>
<polygon fill="black" stroke="black" points="155.78,-46.45 164.88,-41.03 154.39,-39.59 155.78,-46.45"/>
</g>
<!-- upb_msg2 -->
<g id="node3" class="node">
<title>upb_msg2</title>
<path fill="#7fc97f" stroke="black" d="M238,-106C238,-106 163,-106 163,-106 157,-106 151,-100 151,-94 151,-94 151,-82 151,-82 151,-76 157,-70 163,-70 163,-70 238,-70 238,-70 244,-70 250,-76 250,-82 250,-82 250,-94 250,-94 250,-100 244,-106 238,-106"/>
<text text-anchor="middle" x="200.5" y="-84.3" font-family="Times,serif" font-size="14.00">upb Message 2</text>
</g>
<!-- upb_msg&#45;&gt;upb_msg2 -->
<g id="edge2" class="edge">
<title>upb_msg&#45;&gt;upb_msg2</title>
<path fill="none" stroke="black" d="M115.27,-70.9C123.58,-72.59 132.29,-74.36 140.83,-76.09"/>
<polygon fill="black" stroke="black" points="140.24,-79.54 150.74,-78.1 141.63,-72.68 140.24,-79.54"/>
</g>
<!-- upb_msg3 -->
<g id="node4" class="node">
<title>upb_msg3</title>
<path fill="#7fc97f" stroke="black" d="M373,-106C373,-106 298,-106 298,-106 292,-106 286,-100 286,-94 286,-94 286,-82 286,-82 286,-76 292,-70 298,-70 298,-70 373,-70 373,-70 379,-70 385,-76 385,-82 385,-82 385,-94 385,-94 385,-100 379,-106 373,-106"/>
<text text-anchor="middle" x="335.5" y="-84.3" font-family="Times,serif" font-size="14.00">upb Message 3</text>
</g>
<!-- upb_msg2&#45;&gt;upb_msg3 -->
<g id="edge3" class="edge">
<title>upb_msg2&#45;&gt;upb_msg3</title>
<path fill="none" stroke="black" d="M250.27,-88C258.49,-88 267.1,-88 275.55,-88"/>
<polygon fill="black" stroke="black" points="275.74,-91.5 285.74,-88 275.74,-84.5 275.74,-91.5"/>
</g>
</g>
</svg>
</div>
When objects are on separate arenas, it is the user's responsibility to ensure
that there are no dangling pointers.  In the example above, this means Arena 2
must outlive Message 1 and Message 2.

### Integrating GC with upb

In languages with automatic memory management, the goal is to handle all of the
arenas behind the scenes, so that the user does not have to manage them manually
or even know that they exist.

We can achieve this goal if we set up the object graph in a particular way.  The
general strategy is to create wrapper objects around all of the C objects,
including the arena.  Our key goal is to make sure the arena wrapper is not
GC'd until all of the C objects in that arena have become unreachable.

For this example, we will assume we are wrapping upb in Python:

<div align=center><?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"
 "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<!-- Generated by graphviz version 2.43.0 (0)
 -->
<!-- Title: G Pages: 1 -->
<svg width="417pt" height="303pt"
 viewBox="0.00 0.00 417.00 303.00" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<g id="graph0" class="graph" transform="scale(1 1) rotate(0) translate(4 299)">
<title>G</title>
<g id="clust1" class="cluster">
<title>cluster_1</title>
<path fill="gray" stroke="black" d="M29,-98C29,-98 380,-98 380,-98 386,-98 392,-104 392,-110 392,-110 392,-215 392,-215 392,-221 386,-227 380,-227 380,-227 29,-227 29,-227 23,-227 17,-221 17,-215 17,-215 17,-110 17,-110 17,-104 23,-98 29,-98"/>
<text text-anchor="middle" x="204.5" y="-211.8" font-family="Times,serif" font-size="14.00">upb Arena</text>
</g>
<g id="clust2" class="cluster">
<title>cluster_python</title>
<polygon fill="transparent" stroke="transparent" points="8,-235 8,-287 401,-287 401,-235 8,-235"/>
</g>
<g id="clust6" class="cluster">
<title>cluster_01</title>
<polygon fill="transparent" stroke="transparent" points="8.5,-8 8.5,-90 239.5,-90 239.5,-8 8.5,-8"/>
</g>
<!-- upb_msg -->
<g id="node1" class="node">
<title>upb_msg</title>
<path fill="#7fc97f" stroke="black" d="M102,-169C102,-169 37,-169 37,-169 31,-169 25,-163 25,-157 25,-157 25,-145 25,-145 25,-139 31,-133 37,-133 37,-133 102,-133 102,-133 108,-133 114,-139 114,-145 114,-145 114,-157 114,-157 114,-163 108,-169 102,-169"/>
<text text-anchor="middle" x="69.5" y="-147.3" font-family="Times,serif" font-size="14.00">upb Message</text>
</g>
<!-- upb_array -->
<g id="node2" class="node">
<title>upb_array</title>
<path fill="#7fc97f" stroke="black" d="M363,-142C363,-142 316,-142 316,-142 310,-142 304,-136 304,-130 304,-130 304,-118 304,-118 304,-112 310,-106 316,-106 316,-106 363,-106 363,-106 369,-106 375,-112 375,-118 375,-118 375,-130 375,-130 375,-136 369,-142 363,-142"/>
<text text-anchor="middle" x="339.5" y="-120.3" font-family="Times,serif" font-size="14.00">upb Array</text>
</g>
<!-- upb_msg&#45;&gt;upb_array -->
<g id="edge1" class="edge">
<title>upb_msg&#45;&gt;upb_array</title>
<path fill="none" stroke="black" stroke-dasharray="5,2" d="M114.33,-139.43C128.49,-136.14 144.32,-132.93 159,-131 204.59,-125.01 257.21,-123.6 293.64,-123.48"/>
<polygon fill="black" stroke="black" points="293.93,-126.98 303.93,-123.48 293.93,-119.98 293.93,-126.98"/>
</g>
<!-- upb_msg2 -->
<g id="node3" class="node">
<title>upb_msg2</title>
<path fill="#7fc97f" stroke="black" d="M372,-196C372,-196 307,-196 307,-196 301,-196 295,-190 295,-184 295,-184 295,-172 295,-172 295,-166 301,-160 307,-160 307,-160 372,-160 372,-160 378,-160 384,-166 384,-172 384,-172 384,-184 384,-184 384,-190 378,-196 372,-196"/>
<text text-anchor="middle" x="339.5" y="-174.3" font-family="Times,serif" font-size="14.00">upb Message</text>
</g>
<!-- upb_msg&#45;&gt;upb_msg2 -->
<g id="edge2" class="edge">
<title>upb_msg&#45;&gt;upb_msg2</title>
<path fill="none" stroke="black" stroke-dasharray="5,2" d="M108.38,-169.06C123.72,-175.44 141.85,-181.82 159,-185 200.72,-192.74 248.8,-190.27 284.7,-186.27"/>
<polygon fill="black" stroke="black" points="285.44,-189.7 294.95,-185.04 284.61,-182.75 285.44,-189.7"/>
</g>
<!-- dummy -->
<!-- dummy&#45;&gt;upb_array -->
<!-- dummy&#45;&gt;upb_msg2 -->
<!-- py_upb_msg -->
<g id="node5" class="node">
<title>py_upb_msg</title>
<path fill="#beaed4" stroke="black" d="M111,-279C111,-279 28,-279 28,-279 22,-279 16,-273 16,-267 16,-267 16,-255 16,-255 16,-249 22,-243 28,-243 28,-243 111,-243 111,-243 117,-243 123,-249 123,-255 123,-255 123,-267 123,-267 123,-273 117,-279 111,-279"/>
<text text-anchor="middle" x="69.5" y="-257.3" font-family="Times,serif" font-size="14.00">Python Message</text>
</g>
<!-- py_upb_msg&#45;&gt;upb_msg -->
<g id="edge3" class="edge">
<title>py_upb_msg&#45;&gt;upb_msg</title>
<path fill="none" stroke="black" stroke-dasharray="5,2" d="M69.5,-242.68C69.5,-221.49 69.5,-200.3 69.5,-179.11"/>
<polygon fill="black" stroke="black" points="73,-179.05 69.5,-169.05 66,-179.05 73,-179.05"/>
</g>
<!-- py_upb_arena -->
<g id="node7" class="node">
<title>py_upb_arena</title>
<path fill="#beaed4" stroke="black" d="M238,-279C238,-279 171,-279 171,-279 165,-279 159,-273 159,-267 159,-267 159,-255 159,-255 159,-249 165,-243 171,-243 171,-243 238,-243 238,-243 244,-243 250,-249 250,-255 250,-255 250,-267 250,-267 250,-273 244,-279 238,-279"/>
<text text-anchor="middle" x="204.5" y="-257.3" font-family="Times,serif" font-size="14.00">Python Arena</text>
</g>
<!-- py_upb_msg&#45;&gt;py_upb_arena -->
<g id="edge6" class="edge">
<title>py_upb_msg&#45;&gt;py_upb_arena</title>
<path fill="none" stroke="#008b45" d="M123.07,-261C131.43,-261 140.1,-261 148.51,-261"/>
<polygon fill="#008b45" stroke="#008b45" points="148.62,-264.5 158.62,-261 148.62,-257.5 148.62,-264.5"/>
</g>
<!-- py_upb_msg2 -->
<g id="node6" class="node">
<title>py_upb_msg2</title>
<path fill="#beaed4" stroke="black" d="M381,-279C381,-279 298,-279 298,-279 292,-279 286,-273 286,-267 286,-267 286,-255 286,-255 286,-249 292,-243 298,-243 298,-243 381,-243 381,-243 387,-243 393,-249 393,-255 393,-255 393,-267 393,-267 393,-273 387,-279 381,-279"/>
<text text-anchor="middle" x="339.5" y="-257.3" font-family="Times,serif" font-size="14.00">Python Message</text>
</g>
<!-- py_upb_msg2&#45;&gt;upb_msg2 -->
<g id="edge4" class="edge">
<title>py_upb_msg2&#45;&gt;upb_msg2</title>
<path fill="none" stroke="black" stroke-dasharray="5,2" d="M339.5,-242.76C339.5,-230.63 339.5,-218.49 339.5,-206.35"/>
<polygon fill="black" stroke="black" points="343,-206.16 339.5,-196.16 336,-206.16 343,-206.16"/>
</g>
<!-- py_upb_msg2&#45;&gt;py_upb_arena -->
<g id="edge5" class="edge">
<title>py_upb_msg2&#45;&gt;py_upb_arena</title>
<path fill="none" stroke="#008b45" d="M285.91,-261C277.49,-261 268.77,-261 260.31,-261"/>
<polygon fill="#008b45" stroke="#008b45" points="260.16,-257.5 250.16,-261 260.16,-264.5 260.16,-257.5"/>
</g>
<!-- py_upb_arena&#45;&gt;dummy -->
<g id="edge7" class="edge">
<title>py_upb_arena&#45;&gt;dummy</title>
<path fill="none" stroke="red" d="M204.5,-242.76C204.5,-240.95 204.5,-239.15 204.5,-237.34"/>
<polygon fill="red" stroke="red" points="208,-237 204.5,-227 201,-237 208,-237"/>
</g>
<!-- key -->
<g id="node8" class="node">
<title>key</title>
<text text-anchor="start" x="72.5" y="-63.8" font-family="Times,serif" font-size="14.00">raw ptr</text>
<text text-anchor="start" x="56.5" y="-44.8" font-family="Times,serif" font-size="14.00">unique ptr</text>
<text text-anchor="start" x="26.5" y="-25.8" font-family="Times,serif" font-size="14.00">shared (GC) ptr</text>
</g>
<!-- key2 -->
<g id="node9" class="node">
<title>key2</title>
<text text-anchor="start" x="202.5" y="-63.8" font-family="Times,serif" font-size="14.00"> </text>
<text text-anchor="start" x="202.5" y="-44.8" font-family="Times,serif" font-size="14.00"> </text>
<text text-anchor="start" x="202.5" y="-25.8" font-family="Times,serif" font-size="14.00"> </text>
</g>
<!-- key&#45;&gt;key2 -->
<g id="edge10" class="edge">
<title>key:e&#45;&gt;key2:w</title>
<path fill="none" stroke="black" stroke-dasharray="5,2" d="M115.5,-68C149.19,-68 160.08,-68 189.31,-68"/>
<polygon fill="black" stroke="black" points="189.5,-71.5 199.5,-68 189.5,-64.5 189.5,-71.5"/>
</g>
<!-- key&#45;&gt;key2 -->
<g id="edge11" class="edge">
<title>key:e&#45;&gt;key2:w</title>
<path fill="none" stroke="red" d="M115.5,-48C149.19,-48 160.08,-48 189.31,-48"/>
<polygon fill="red" stroke="red" points="189.5,-51.5 199.5,-48 189.5,-44.5 189.5,-51.5"/>
</g>
<!-- key&#45;&gt;key2 -->
<g id="edge12" class="edge">
<title>key:e&#45;&gt;key2:w</title>
<path fill="none" stroke="#008b45" d="M115.5,-29C149.19,-29 160.08,-29 189.31,-29"/>
<polygon fill="#008b45" stroke="#008b45" points="189.5,-32.5 199.5,-29 189.5,-25.5 189.5,-32.5"/>
</g>
<!-- key2&#45;&gt;upb_msg -->
</g>
</svg>
</div>
In this example we have three different kinds of pointers:

* **raw ptr**: This is a pointer that carries no ownership.
* **unique ptr**: This is a pointer has *unique ownership* of the target.  The owner
  will free the target in its destructor (or finalizer, or cleaner).  There can
  only be a single unique pointer to a given object.
* **shared (GC) ptr**: This is a pointer that has *shared ownership* of the
  target.  Many objects can point to the target, and the target will be deleted
  only when all such references are gone.  In a runtime with automatic memory
  management (GC), this is a reference that participates in GC.  In Python such
  references use reference counting, but in other VMs they may use mark and
  sweep or some other form of GC instead.

The Python Message wrappers have only raw pointers to the underlying message,
but they contain a shared pointer to the arena that will ensure that the raw
pointer remains valid.  Only when all message wrapper objects are destroyed
will the Python Arena become unreachable, and the upb arena ultimately freed.

### Links between arenas with "Fuse"

The design given above works well for objects that live in a single arena. But
what if a user wants to create a link between two objects in different arenas?

TODO

## UTF-8 vs. UTF-16

TODO

## Object Cache

TODO
