// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

extern crate proc_macro;

use quote::{format_ident, quote, ToTokens};
use syn::{
    parse::ParseStream, parse_macro_input, parse_quote, punctuated::Punctuated, Error, Expr,
    ExprArray, ExprPath, ExprStruct, ExprTuple, FieldValue, Ident, Member, Path, QSelf, Result,
    Stmt, Token, Type, TypePath,
};

/// proto! enables the use of Rust struct initialization syntax to create
/// protobuf messages. The macro does its best to try and detect the
/// initialization of submessages, but it is only able to do so while the
/// submessage is being defined as part of the original struct literal.
/// Introducing an expression using () or {} as the value of a field will
/// require another call to this macro in order to return a submessage
/// literal.
///
/// ```rust,ignore
/// /*
/// Given the following proto definition:
/// message Data {
///     int32 number = 1;
///     string letters = 2;
///     Data nested = 3;
/// }
/// */
/// use protobuf_proc_macro::proto_proc;
/// let message = proto_proc!(Data {
///     number: 42,
///     letters: "Hello World",
///     nested: Data {
///         number: {
///             let x = 100;
///             x + 1
///         }
///     }
/// });
/// ```
#[proc_macro]
pub fn proto_proc(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    let result = parse_macro_input!(input with parse_and_expand_top_level_struct);
    result.to_token_stream().into()
}

fn parse_and_expand_top_level_struct(input: ParseStream) -> Result<Expr> {
    expand_struct(input.parse()?, EnclosingContext::TopLevel)
}

/// The context in which an expression (struct or array literal) is being
/// expanded.
///
/// This is needed because it subtly affects the generated code. For example,
/// when expanding a nested message, the field is mutated in-place, but when
/// expanding a message inside an array, a new message is created.
#[derive(Clone)]
enum EnclosingContext {
    /// The current expression is the top-level message, directly inside the
    /// `proto!` invocation.
    TopLevel,

    /// The current expression will be assigned to a field of a parent message.
    Struct {
        /// The name of the enclosing field.
        field: Ident,
    },

    /// The current expression is an element of a repeated field (i.e. array).
    Array {
        /// The name of the repeated field.
        repeated_field: Ident,
    },

    /// The current expression is (key, value) tuple literal for a map field.
    Map { map_field: Ident },
}

impl EnclosingContext {
    pub fn repeated_field(&self) -> Ident {
        match self {
            EnclosingContext::Array { repeated_field } => repeated_field.clone(),
            _ => panic!("Can only get the repeated field of an array context"),
        }
    }
}

fn expand_struct(
    ExprStruct { attrs, qself, path, fields, rest, .. }: ExprStruct,
    enclosing_context: EnclosingContext,
) -> Result<Expr> {
    if let Some(attr) = attrs.first() {
        return Err(Error::new_spanned(attr, "unsupported syntax"));
    }

    let merge = rest.map(|rest| {
        quote! {
            ::protobuf::MergeFrom::merge_from(&mut this, #rest);
        }
    });

    let fields = expand_struct_fields(fields)?;
    let this_type = expand_struct_type(qself.clone(), path.clone(), enclosing_context.clone());
    let (head, tail) =
        expand_struct_head_tail(qself.clone(), path.clone(), enclosing_context.clone())?;

    Ok(parse_quote! {{
        let mut this: #this_type = #head;
        #merge
        #(#fields)*
        #tail
    }})
}

fn expand_struct_fields(fields: Punctuated<FieldValue, Token![,]>) -> Result<Vec<Stmt>> {
    fields
        .into_iter()
        .map(|field| {
            let Member::Named(ident) = field.member else {
                return Err(Error::new_spanned(field, "field must be an identifier, not an index"));
            };
            let set_method = format_ident!("set_{}", ident);
            let enclosing_context = EnclosingContext::Struct { field: ident };
            let expr = match field.expr {
                Expr::Struct(struct_) => {
                    // In a nested message, the value will be mutated in-place, so there is no need
                    // to call the top-level setter.
                    let expanded = expand_struct(struct_, enclosing_context)?;
                    return Ok(parse_quote!(#expanded));
                }
                Expr::Array(array) => expand_array(array, enclosing_context)?,
                expr => expr,
            };
            Ok(parse_quote! {
                this.#set_method(#expr);
            })
        })
        .collect()
}

fn expand_struct_type(
    qself: Option<QSelf>,
    path: Path,
    enclosing_context: EnclosingContext,
) -> Type {
    if should_infer_message_type(&qself, &path) {
        return parse_quote!(_);
    }

    let type_path = TypePath { qself, path };
    if let EnclosingContext::Struct { .. } = enclosing_context {
        // Nested messages use a mutable view type.
        parse_quote!(::protobuf::Mut<'_, #type_path>)
    } else {
        parse_quote!(#type_path)
    }
}

/// Builds two expressions: one that initializes the message, and another that
/// returns it (if any).
///
/// If the enclosing context is another message, the child message is mutated
/// in-place, so the returning expression is `None`.
fn expand_struct_head_tail(
    qself: Option<QSelf>,
    path: Path,
    enclosing_context: EnclosingContext,
) -> Result<(Expr, Option<Expr>)> {
    match enclosing_context {
        EnclosingContext::TopLevel => {
            if should_infer_message_type(&qself, &path) {
                Err(Error::new_spanned(path, "message type must be specified explicitly"))
            } else {
                let path = ExprPath { attrs: Vec::new(), qself, path };
                Ok((parse_quote!(#path::new()), Some(parse_quote!(this))))
            }
        }
        EnclosingContext::Struct { field } => {
            // In a nested message, mutate the field in-place.
            let field_mut = format_ident!("{}_mut", field);
            Ok((parse_quote!(this.#field_mut()), None))
        }
        EnclosingContext::Array { repeated_field } => {
            // In a message inside an array, create a new message and return it.
            // The type trickery is used to infer the message type from the repeated wrapper.
            Ok((
                parse_quote!(::protobuf::__internal::get_repeated_default_value(
                    ::protobuf::__internal::Private,
                    this.#repeated_field()
                )),
                Some(parse_quote!(this)),
            ))
        }
        EnclosingContext::Map { map_field } => {
            // In a message inside a key value tuple, create a new message and return it.
            // The type trickery is used to infer the message type from the map wrapper.
            Ok((
                parse_quote!(::protobuf::__internal::get_map_default_value(
                    ::protobuf::__internal::Private,
                    this.#map_field()
                )),
                Some(parse_quote!(this)),
            ))
        }
    }
}

/// Returns `true` if the given path is `__` (two underscores), which indicates
/// an inferred type.
fn should_infer_message_type(qself: &Option<QSelf>, path: &Path) -> bool {
    qself.is_none() && path.get_ident().is_some_and(|ident| *ident == "__")
}

fn expand_array(array: ExprArray, enclosing_context: EnclosingContext) -> Result<Expr> {
    if let Some(attr) = array.attrs.first() {
        return Err(Error::new_spanned(attr, "unsupported syntax"));
    }

    let enclosing_context = EnclosingContext::Array {
        repeated_field: match enclosing_context {
            EnclosingContext::Struct { field } => field,
            EnclosingContext::TopLevel
            | EnclosingContext::Array { .. }
            | EnclosingContext::Map { .. } => {
                return Err(Error::new_spanned(array, "arrays must be nested inside a message"))
            }
        },
    };

    let array = array
        .elems
        .into_iter()
        .map(|elem| match elem {
            Expr::Struct(struct_) => expand_struct(struct_, enclosing_context.clone()),
            Expr::Array(array) => expand_array(array, enclosing_context.clone()),
            Expr::Tuple(tuple) => expand_tuple(
                tuple,
                EnclosingContext::Map { map_field: enclosing_context.repeated_field() },
            ),
            expr => Ok(expr),
        })
        .collect::<Result<Vec<Expr>>>()?;

    Ok(parse_quote!([#(#array),*].into_iter()))
}

fn expand_tuple(tuple: ExprTuple, enclosing_context: EnclosingContext) -> Result<Expr> {
    if let Some(attr) = tuple.attrs.first() {
        return Err(Error::new_spanned(attr, "unsupported syntax"));
    }

    if tuple.elems.len() != 2 {
        return Err(Error::new_spanned(tuple, "Map tuple literals must have exactly two elements"));
    }

    let key = tuple.elems[0].clone();
    let value = match &tuple.elems[1] {
        Expr::Struct(struct_) => expand_struct(struct_.clone(), enclosing_context.clone()),
        expr => Ok(expr.clone()),
    }?;
    Ok(parse_quote!((#key, #value)))
}
