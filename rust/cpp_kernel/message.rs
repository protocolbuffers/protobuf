use super::*;

unsafe extern "C" {
    pub fn proto2_rust_Message_delete(m: RawMessage);
    pub fn proto2_rust_Message_clear(m: RawMessage);
    pub fn proto2_rust_Message_parse(m: RawMessage, input: PtrAndLen) -> bool;
    pub fn proto2_rust_Message_parse_dont_enforce_required(m: RawMessage, input: PtrAndLen)
        -> bool;
    pub fn proto2_rust_Message_serialize(m: RawMessage, output: &mut SerializedData) -> bool;
    pub fn proto2_rust_Message_serialized_len(m: RawMessage) -> usize;
    pub fn proto2_rust_Message_copy_from(dst: RawMessage, src: RawMessage);
    pub fn proto2_rust_Message_merge_from(dst: RawMessage, src: RawMessage);
    pub fn proto2_rust_Message_get_descriptor(m: RawMessage) -> *const std::ffi::c_void;
}

unsafe extern "C" {
    /// From compare.h
    ///
    /// # Safety
    /// - `raw1` and `raw2` legally dereferenceable MessageLite* pointers.
    #[link_name = "proto2_rust_messagelite_equals"]
    pub fn raw_message_equals(raw1: RawMessage, raw2: RawMessage) -> bool;
}

#[derive(Debug)]
#[doc(hidden)]
#[repr(transparent)]
pub struct OwnedMessageInner<T> {
    raw: RawMessage,
    _phantom: PhantomData<T>,
}

impl<T: Message> OwnedMessageInner<T> {
    /// # Safety
    /// - `raw` must point to a message of type `T` and outlive `Self`.
    pub unsafe fn wrap_raw(raw: RawMessage) -> Self {
        OwnedMessageInner { raw, _phantom: PhantomData }
    }

    pub fn raw(&self) -> RawMessage {
        self.raw
    }
}

/// Mutators that point to their original message use this to do so.
///
/// Since C++ messages manage their own memory, this can just copy the
/// `RawMessage` instead of referencing an arena like UPB must.
///
/// The following invariants must be upheld:
///
/// - No concurrent mutation for any two fields in a message: this means
///   mutators cannot be `Send` but are `Sync`.
/// - If there are multiple accessible `Mut` to a single message at a time, they
///   must be different fields, and not be in the same oneof. As such, a `Mut`
///   cannot be `Clone` but *can* reborrow itself with `.as_mut()`, which
///   converts `&'b mut Mut<'a, T>` to `Mut<'b, T>`.
#[derive(Debug)]
#[doc(hidden)]
#[repr(transparent)]
pub struct MessageMutInner<'msg, T> {
    raw: RawMessage,
    _phantom: PhantomData<(&'msg mut (), T)>,
}

impl<'msg, T: Message> MessageMutInner<'msg, T> {
    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn mut_of_owned(msg: &'msg mut OwnedMessageInner<T>) -> Self {
        MessageMutInner { raw: msg.raw, _phantom: PhantomData }
    }

    /// # Safety
    /// - The underlying pointer must be mutable, of type `T` and live for the lifetime 'msg.
    pub unsafe fn wrap_raw(raw: RawMessage) -> Self {
        MessageMutInner { raw, _phantom: PhantomData }
    }

    pub fn from_parent<ParentT: Message>(
        _parent_msg: MessageMutInner<'msg, ParentT>,
        message_field_ptr: RawMessage,
    ) -> Self {
        Self { raw: message_field_ptr, _phantom: PhantomData }
    }

    pub fn raw(&self) -> RawMessage {
        self.raw
    }

    pub fn as_view(&self) -> MessageViewInner<'msg, T> {
        MessageViewInner { raw: self.raw, _phantom: PhantomData }
    }

    pub fn reborrow<'shorter>(&mut self) -> MessageMutInner<'shorter, T>
    where
        'msg: 'shorter,
    {
        MessageMutInner { raw: self.raw, _phantom: PhantomData }
    }
}

#[derive(Debug)]
#[doc(hidden)]
#[repr(transparent)]
pub struct MessageViewInner<'msg, T> {
    raw: RawMessage,
    _phantom: PhantomData<(&'msg (), T)>,
}

impl<'msg, T: Message> Clone for MessageViewInner<'msg, T> {
    fn clone(&self) -> Self {
        *self
    }
}
impl<'msg, T: Message> Copy for MessageViewInner<'msg, T> {}

impl<'msg, T: Message> MessageViewInner<'msg, T> {
    /// # Safety
    /// - The underlying pointer must of type `T` and live for the lifetime 'msg.
    pub unsafe fn wrap_raw(raw: RawMessage) -> Self {
        MessageViewInner { raw, _phantom: PhantomData }
    }

    pub fn view_of_owned(msg: &'msg OwnedMessageInner<T>) -> Self {
        MessageViewInner { raw: msg.raw, _phantom: PhantomData }
    }

    pub fn view_of_mut(msg: MessageMutInner<'msg, T>) -> Self {
        MessageViewInner { raw: msg.raw, _phantom: PhantomData }
    }

    pub fn raw(&self) -> RawMessage {
        self.raw
    }
}

/// Internal-only trait to support blanket impls that need const access to raw messages
/// on codegen. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait CppGetRawMessage: SealedInternal {
    fn get_raw_message(&self, _private: Private) -> RawMessage;
}

// The generated code only implements this trait on View proxies, so we use a blanket
// implementation for owned messages and Mut proxies.
unsafe impl<T> CppGetRawMessage for T
where
    Self: AsMut + AsView,
    for<'a> View<'a, <Self as AsView>::Proxied>: CppGetRawMessage,
{
    fn get_raw_message(&self, _private: Private) -> RawMessage {
        self.as_view().get_raw_message(_private)
    }
}

/// Internal-only trait to support blanket impls that need mutable access to raw messages
/// on codegen. Must not be implemented on View proxies. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait CppGetRawMessageMut: SealedInternal {
    fn get_raw_message_mut(&mut self, _private: Private) -> RawMessage;
}

// The generated code only implements this trait on Mut proxies, so we use a blanket implementation
// for owned messages.
unsafe impl<T> CppGetRawMessageMut for T
where
    Self: MutProxied,
    for<'a> Mut<'a, Self>: CppGetRawMessageMut,
{
    fn get_raw_message_mut(&mut self, _private: Private) -> RawMessage {
        self.as_mut().get_raw_message_mut(_private)
    }
}

pub trait KernelMessage: CppGetRawMessage + CppGetRawMessageMut + OwnedMessageInterop {}
impl<T: CppGetRawMessage + CppGetRawMessageMut + OwnedMessageInterop> KernelMessage for T {}

pub trait KernelMessageView<'msg>:
    CppGetRawMessage + MessageViewInterop<'msg> + AsView + From<MessageViewInner<'msg, Self::KMessage>>
{
    type KMessage;
}

impl<'msg, T> KernelMessageView<'msg> for T
where
    T: CppGetRawMessage
        + MessageViewInterop<'msg>
        + AsView
        + From<MessageViewInner<'msg, T::Proxied>>,
{
    type KMessage = T::Proxied;
}

pub trait KernelMessageMut<'msg>:
    CppGetRawMessageMut + MessageMutInterop<'msg> + AsMut + From<MessageMutInner<'msg, Self::KMessage>>
{
    type KMessage;
}

impl<'msg, T> KernelMessageMut<'msg> for T
where
    T: CppGetRawMessageMut
        + MessageMutInterop<'msg>
        + AsMut
        + From<MessageMutInner<'msg, T::MutProxied>>,
{
    type KMessage = T::MutProxied;
}

/// Message equality definition which may have both false-negatives and false-positives in the face
/// of unknown fields.
///
/// This behavior is deliberately held back from being exposed as an `Eq` trait for messages. The
/// reason is that it is impossible to properly compare unknown fields for message equality, since
/// without the schema you cannot know how to interpret the wire format properly for comparison.
///
/// False negative cases (where message_eq will return false on unknown fields where it
/// would return true if the fields were known) are common and will occur in production: for
/// example, as map and repeated fields look exactly the same, map field order is unstable, the
/// comparison cannot know to treat it as unordered and will return false when it was the same
/// map but in a different order.
///
/// False positives cases (where message_eq will return true on unknown fields where it would have
/// return false if the fields were known) are possible but uncommon in practice. One example
/// of this direction can occur if two fields are defined in the same oneof and both are present on
/// the wire but in opposite order, without the schema these messages appear equal but with the
/// schema they are not-equal.
///
/// This lossy behavior in the face of unknown fields is especially problematic in the face of
/// extensions and other treeshaking behaviors where a given field being known or not to binary is a
/// spooky-action-at-a-distance behavior, which may lead to surprising changes in outcome in
/// equality tests based on changes made arbitrarily distant from the code performing the equality
/// check.
///
/// Broadly this is recommended for use in tests (where unknown field behaviors are rarely a
/// concern), and in limited/targeted cases where the lossy behavior in the face of unknown fields
/// behavior is unlikely to be a problem.
pub fn message_eq<T>(a: &T, b: &T) -> bool
where
    T: AsView + Debug,
    for<'a> View<'a, <T as AsView>::Proxied>: CppGetRawMessage,
{
    unsafe {
        raw_message_equals(
            a.as_view().get_raw_message(Private),
            b.as_view().get_raw_message(Private),
        )
    }
}

impl<T> MatcherEq for T
where
    Self: AsView + Debug,
    for<'a> View<'a, <Self as AsView>::Proxied>: CppGetRawMessage,
{
    fn matches(&self, o: &Self) -> bool {
        message_eq(self, o)
    }
}

impl<T: CppGetRawMessageMut> Clear for T {
    fn clear(&mut self) {
        unsafe { proto2_rust_Message_clear(self.get_raw_message_mut(Private)) }
    }
}

impl<T: CppGetRawMessageMut> ClearAndParse for T {
    fn clear_and_parse(&mut self, data: &[u8]) -> Result<(), ParseError> {
        unsafe { proto2_rust_Message_parse(self.get_raw_message_mut(Private), data.into()) }
            .then_some(())
            .ok_or(ParseError)
    }

    fn clear_and_parse_dont_enforce_required(&mut self, data: &[u8]) -> Result<(), ParseError> {
        unsafe {
            proto2_rust_Message_parse_dont_enforce_required(
                self.get_raw_message_mut(Private),
                data.into(),
            )
        }
        .then_some(())
        .ok_or(ParseError)
    }
}

impl<T: CppGetRawMessage> Serialize for T {
    fn serialize(&self) -> Result<Vec<u8>, SerializeError> {
        let mut serialized_data = SerializedData::new();
        let success = unsafe {
            proto2_rust_Message_serialize(self.get_raw_message(Private), &mut serialized_data)
        };
        if success {
            Ok(serialized_data.into_vec())
        } else {
            Err(SerializeError)
        }
    }

    fn serialized_len(&self) -> usize {
        unsafe { proto2_rust_Message_serialized_len(self.get_raw_message(Private)) }
    }
}

impl<T> TakeFrom for T
where
    Self: CopyFrom + AsMut,
    for<'a> Mut<'a, <Self as AsMut>::MutProxied>: Clear,
{
    fn take_from(&mut self, mut src: impl AsMut<MutProxied = Self::Proxied>) {
        let mut src = src.as_mut();
        // TODO: b/393559271 - Optimize this copy out.
        CopyFrom::copy_from(self, AsView::as_view(&src));
        Clear::clear(&mut src);
    }
}

impl<T> CopyFrom for T
where
    Self: AsMut,
    for<'a> View<'a, Self::Proxied>: CppGetRawMessage,
    for<'a> Mut<'a, Self::Proxied>: CppGetRawMessageMut,
{
    fn copy_from(&mut self, src: impl AsView<Proxied = Self::Proxied>) {
        unsafe {
            proto2_rust_Message_copy_from(
                self.as_mut().get_raw_message_mut(Private),
                src.as_view().get_raw_message(Private),
            );
        }
    }
}

impl<T> MergeFrom for T
where
    Self: AsMut,
    for<'a> View<'a, Self::Proxied>: CppGetRawMessage,
    for<'a> Mut<'a, Self::Proxied>: CppGetRawMessageMut,
{
    fn merge_from(&mut self, src: impl AsView<Proxied = Self::Proxied>) {
        // SAFETY: self and src are both valid `T`s.
        unsafe {
            proto2_rust_Message_merge_from(
                self.as_mut().get_raw_message_mut(Private),
                src.as_view().get_raw_message(Private),
            );
        }
    }
}
