" Protocol Buffers - Google's data interchange format
" Copyright 2008 Google Inc.  All rights reserved.
"
" Use of this source code is governed by a BSD-style
" license that can be found in the LICENSE file or at
" https://developers.google.com/open-source/licenses/bsd

" This is the Vim syntax file for Google Protocol Buffers.
"
" Usage:
"
" 1. cp proto.vim ~/.vim/syntax/
" 2. Add the following to ~/.vimrc:
"
" augroup filetype
"   au! BufRead,BufNewFile *.proto setfiletype proto
" augroup end
"
" Or just create a new file called ~/.vim/ftdetect/proto.vim with the
" previous lines on it.

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case match

syn keyword pbTodo       contained TODO FIXME XXX
syn cluster pbCommentGrp contains=pbTodo

syn keyword pbSyntax     syntax import option edition
syn keyword pbStructure  package message group oneof
syn keyword pbRepeat     optional required repeated
syn keyword pbDefault    default
syn keyword pbExtend     extend extensions to max reserved
syn keyword pbRPC        service rpc returns
syn keyword pbVisibility local export

syn keyword pbType      int32 int64 uint32 uint64 sint32 sint64
syn keyword pbType      fixed32 fixed64 sfixed32 sfixed64
syn keyword pbType      float double bool string bytes
syn keyword pbTypedef   enum
syn keyword pbBool      true false

syn match   pbInt     /-\?\<\d\+\>/
syn match   pbInt     /\<0[xX]\x+\>/
syn match   pbFloat   /\<-\?\d*\(\.\d*\)\?/
syn region  pbComment start="\/\*" end="\*\/" contains=@pbCommentGrp,@Spell
syn region  pbComment start="//" skip="\\$" end="$" keepend contains=@pbCommentGrp,@Spell
syn region  pbString  start=/"/ skip=/\\./ end=/"/ contains=@Spell
syn region  pbString  start=/'/ skip=/\\./ end=/'/ contains=@Spell

if version >= 508 || !exists("did_proto_syn_inits")
  if version < 508
    let did_proto_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink pbTodo         Todo

  HiLink pbSyntax       Include
  HiLink pbStructure    Structure
  HiLink pbRepeat       Repeat
  HiLink pbDefault      Keyword
  HiLink pbExtend       Keyword
  HiLink pbRPC          Keyword
  HiLink pbVisibility   Keyword
  HiLink pbType         Type
  HiLink pbTypedef      Typedef
  HiLink pbBool         Boolean

  HiLink pbInt          Number
  HiLink pbFloat        Float
  HiLink pbComment      Comment
  HiLink pbString       String

  delcommand HiLink
endif

let b:current_syntax = "proto"
