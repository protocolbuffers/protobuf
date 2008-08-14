Readme for the C#/.NET implementation of Protocol Buffers

Copyright 2008 Google Inc.
http://code.google.com/apis/protocolbuffers/
and
http://github.com/jskeet/dotnet-protobufs

(This will eventually be written up into a full tutorial etc.)

Differences with respect to the Java API
----------------------------------------

Many of the changes are obvious "making it more like .NET", but
others are more subtle.

o Properties and indexers are used reasonably extensively.
o newFoo becomes CreateFoo everywhere.
o Classes are broken up much more - for instance, there are
  namespaces for descriptors and field accessors, just to make it
  easier to see what's going on.
o There's a mixture of generic and non-generic code. This
  is interesting (at least if you're a language nerd). Java's generics
  are somewhat different to those of .NET, partly due to type erasure
  but in other ways too. .NET allows types to be overloaded by the
  number of generic parameters, but there's no such thing as the
  "raw" type of a generic type. Combining these two facts, I've
  ended up with two interfaces for messages, and two for builders -
  in each case, a non-generic one and a generic one which derives
  from the generic one. Where the members clash (e.g. IBuilder.Build
  and IBuilder<TMessage>.Build vary only by return type) the
  implementations use explicit interface implementation to provide
  the most useful method in most cases. This is very much like
  the normal implementation of IEnumerable<T> which extends
  IEnumerable. As an aside, this becomes a pain when trying to
  create "the read-only version of this list, whose type I don't
  know but I know it's a List<something>". Oh for mumble types.
o Nested types always end up under a "Types" static class which
  is merely present to avoid name clashes.
o FileDescriptor.FindByName has been made generic to allow simple
  type-safe searching for any nested type.