use super::*;

#[derive(Debug)]
#[doc(hidden)]
pub struct OwnedMessageInner<T> {
    ptr: MessagePtr<T>,
    arena: Arena,
}

impl<T: Message> Default for OwnedMessageInner<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: Message> OwnedMessageInner<T> {
    pub fn new() -> Self {
        let arena = Arena::new();
        let ptr = MessagePtr::new(&arena).expect("alloc should never fail");
        Self { ptr, arena }
    }

    /// # Safety
    /// - The underlying pointer must of type `T` and be allocated on `arena`.
    pub unsafe fn wrap_raw(raw: RawMessage, arena: Arena) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid and of type `T`
        let ptr = unsafe { MessagePtr::wrap(raw) };
        OwnedMessageInner { ptr, arena }
    }

    pub fn ptr_mut(&mut self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn ptr(&self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn raw(&self) -> RawMessage {
        self.ptr.raw()
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound access requires mutable access.
    pub fn arena(&mut self) -> &Arena {
        &self.arena
    }
}

/// Mutators that point to their original message use this to do so.
///
/// Since UPB expects runtimes to manage their own arenas, this needs to have
/// access to an `Arena`.
///
/// This has two possible designs:
/// - Store two pointers here, `RawMessage` and `&'msg Arena`. This doesn't
///   place any restriction on the layout of generated messages and their
///   mutators. This makes a vtable-based mutator three pointers, which can no
///   longer be returned in registers on most platforms.
/// - Store one pointer here, `&'msg OwnedMessageInner`, where `OwnedMessageInner` stores
///   a `RawMessage` and an `Arena`. This would require all generated messages
///   to store `OwnedMessageInner`, and since their mutators need to be able to
///   generate `BytesMut`, would also require `BytesMut` to store a `&'msg
///   OwnedMessageInner` since they can't store an owned `Arena`.
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
pub struct MessageMutInner<'msg, T> {
    pub(crate) ptr: MessagePtr<T>,
    pub(crate) arena: &'msg Arena,
}

impl<'msg, T: Message> MessageMutInner<'msg, T> {
    /// # Safety
    /// - `msg` must be a valid `RawMessage`
    /// - `arena` must hold the memory for `msg`
    pub unsafe fn wrap_raw(raw: RawMessage, arena: &'msg Arena) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid and of type `T`
        let ptr = unsafe { MessagePtr::wrap(raw) };
        MessageMutInner { ptr, arena }
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn mut_of_owned(msg: &'msg mut OwnedMessageInner<T>) -> Self {
        MessageMutInner { ptr: msg.ptr, arena: &msg.arena }
    }

    pub fn from_parent<ParentT>(
        parent_msg: MessageMutInner<'msg, ParentT>,
        ptr: MessagePtr<T>,
    ) -> Self {
        MessageMutInner { ptr, arena: parent_msg.arena }
    }

    pub fn ptr_mut(&mut self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn ptr(&self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn raw(&self) -> RawMessage {
        self.ptr.raw()
    }

    pub fn arena(&self) -> &Arena {
        self.arena
    }

    pub fn as_view(&self) -> MessageViewInner<'msg, T> {
        MessageViewInner { ptr: self.ptr, _phantom: PhantomData }
    }

    pub fn reborrow<'shorter>(&mut self) -> MessageMutInner<'shorter, T>
    where
        'msg: 'shorter,
    {
        Self { ptr: self.ptr, arena: self.arena }
    }
}

#[derive(Debug)]
#[doc(hidden)]
pub struct MessageViewInner<'msg, T> {
    ptr: MessagePtr<T>,
    _phantom: PhantomData<&'msg ()>,
}

impl<'msg, T: Message> Clone for MessageViewInner<'msg, T> {
    fn clone(&self) -> Self {
        *self
    }
}
impl<'msg, T: Message> Copy for MessageViewInner<'msg, T> {}

impl<'msg, T: Message> MessageViewInner<'msg, T> {
    /// # Safety
    /// - The underlying pointer must live as long as `'msg`.
    pub unsafe fn wrap(ptr: MessagePtr<T>) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid
        MessageViewInner { ptr, _phantom: PhantomData }
    }

    /// # Safety
    /// - The underlying pointer must of type `T` and live as long as `'msg`.
    pub unsafe fn wrap_raw(raw: RawMessage) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid and of type `T`
        let ptr = unsafe { MessagePtr::wrap(raw) };
        MessageViewInner { ptr, _phantom: PhantomData }
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn view_of_owned(owned: &'msg OwnedMessageInner<T>) -> Self {
        MessageViewInner { ptr: owned.ptr, _phantom: PhantomData }
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn view_of_mut(msg_mut: MessageMutInner<'msg, T>) -> Self {
        MessageViewInner { ptr: msg_mut.ptr, _phantom: PhantomData }
    }

    pub fn ptr(&self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn raw(&self) -> RawMessage {
        self.ptr.raw()
    }
}

/// The scratch size of 64 KiB matches the maximum supported size that a
/// upb_Message can possibly be.
const UPB_SCRATCH_SPACE_BYTES: usize = 65_536;

/// Holds a zero-initialized block of memory for use by upb.
///
/// By default, if a message is not set in cpp, a default message is created.
/// upb departs from this and returns a null ptr. However, since contiguous
/// chunks of memory filled with zeroes are legit messages from upb's point of
/// view, we can allocate a large block and refer to that when dealing
/// with readonly access.
#[repr(C, align(8))] // align to UPB_MALLOC_ALIGN = 8
pub(crate) struct ScratchSpace([u8; UPB_SCRATCH_SPACE_BYTES]);
impl ScratchSpace {
    pub fn zeroed_block() -> RawMessage {
        static ZEROED_BLOCK: ScratchSpace = ScratchSpace([0; UPB_SCRATCH_SPACE_BYTES]);
        NonNull::from(&ZEROED_BLOCK).cast()
    }
}

impl<T: Message> Default for MessageViewInner<'static, T> {
    fn default() -> Self {
        unsafe {
            // SAFETY:
            // - `ScratchSpace::zeroed_block()` is a valid const `RawMessage` for all possible T.
            // - `ScratchSpace::zeroed_block()' has 'static lifetime.
            Self::wrap_raw(ScratchSpace::zeroed_block())
        }
    }
}

/// Internal-only trait to support blanket impls that need const access to raw messages
/// on codegen. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait UpbGetMessagePtr: SealedInternal {
    type Msg: AssociatedMiniTable + Message;

    fn get_ptr(&self, _private: Private) -> MessagePtr<Self::Msg>;
}

/// Internal-only trait to support blanket impls that need mutable access to raw messages
/// on codegen. Must not be implemented on View proxies. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait UpbGetMessagePtrMut: SealedInternal {
    type Msg: AssociatedMiniTable + Message;

    fn get_ptr_mut(&mut self, _private: Private) -> MessagePtr<Self::Msg>;
}

/// Internal-only trait to support blanket impls that need const access to raw messages
/// on codegen. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait UpbGetArena: SealedInternal {
    fn get_arena(&mut self, _private: Private) -> &Arena;
}

pub trait KernelMessage:
    AssociatedMiniTable + UpbGetArena + UpbGetMessagePtr + UpbGetMessagePtrMut + OwnedMessageInterop
{
}

impl<T> KernelMessage for T where
    T: AssociatedMiniTable
        + UpbGetArena
        + UpbGetMessagePtr
        + UpbGetMessagePtrMut
        + OwnedMessageInterop
{
}

pub trait KernelMessageView<'msg>:
    UpbGetMessagePtr + MessageViewInterop<'msg> + From<MessageViewInner<'msg, Self::KMessage>>
{
    type KMessage;
}

impl<'msg, T> KernelMessageView<'msg> for T
where
    T: UpbGetMessagePtr + MessageViewInterop<'msg> + From<MessageViewInner<'msg, T::Msg>>,
{
    type KMessage = T::Msg;
}

pub trait KernelMessageMut<'msg>:
    UpbGetMessagePtr
    + UpbGetMessagePtrMut
    + UpbGetArena
    + MessageMutInterop<'msg>
    + From<MessageMutInner<'msg, Self::KMessage>>
{
    type KMessage;
}
impl<'msg, T> KernelMessageMut<'msg> for T
where
    T: UpbGetMessagePtr
        + UpbGetMessagePtrMut
        + UpbGetArena
        + MessageMutInterop<'msg>
        + From<MessageMutInner<'msg, <T as UpbGetMessagePtr>::Msg>>,
{
    type KMessage = <T as UpbGetMessagePtr>::Msg;
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
    <T as AsView>::Proxied: AssociatedMiniTable,
    for<'a> View<'a, <T as AsView>::Proxied>: UpbGetMessagePtr,
{
    unsafe {
        upb_Message_IsEqual(
            a.as_view().get_ptr(Private).raw(),
            b.as_view().get_ptr(Private).raw(),
            <T as AsView>::Proxied::mini_table(),
            0,
        )
    }
}

impl<T> MatcherEq for T
where
    Self: AsView + Debug,
    <Self as AsView>::Proxied: AssociatedMiniTable,
    for<'a> View<'a, <Self as AsView>::Proxied>: UpbGetMessagePtr,
{
    fn matches(&self, o: &Self) -> bool {
        message_eq(self, o)
    }
}

impl<T: UpbGetMessagePtrMut> Clear for T {
    fn clear(&mut self) {
        unsafe { self.get_ptr_mut(Private).clear() }
    }
}

fn clear_and_parse_helper<T>(
    msg: &mut T,
    data: &[u8],
    decode_options: i32,
) -> Result<(), ParseError>
where
    T: UpbGetMessagePtrMut + UpbGetArena,
{
    Clear::clear(msg);
    // SAFETY:
    // - `msg` is a valid mutable message.
    // - `mini_table` is the one associated with `msg`
    // - `msg.arena().raw()` is held for the same lifetime as `msg`.
    unsafe {
        upb::wire::decode_with_options(
            data,
            msg.get_ptr_mut(Private),
            generated_extension_registry().as_ptr(),
            msg.get_arena(Private),
            decode_options,
        )
    }
    .map(|_| ())
    .map_err(|_| ParseError)
}

impl<T> ClearAndParse for T
where
    Self: UpbGetMessagePtrMut + UpbGetArena,
{
    fn clear_and_parse(&mut self, data: &[u8]) -> Result<(), ParseError> {
        clear_and_parse_helper(self, data, upb::wire::decode_options::CHECK_REQUIRED)
    }

    fn clear_and_parse_dont_enforce_required(&mut self, data: &[u8]) -> Result<(), ParseError> {
        clear_and_parse_helper(self, data, 0)
    }
}

impl<T> Serialize for T
where
    Self: UpbGetMessagePtr,
{
    fn serialize(&self) -> Result<Vec<u8>, SerializeError> {
        //~ TODO: This discards the info we have about the reason
        //~ of the failure, we should try to keep it instead.
        upb::wire::encode(self.get_ptr(Private)).map_err(|_| SerializeError)
    }

    fn serialized_len(&self) -> usize {
        upb::wire::byte_size(self.get_ptr(Private))
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
    Self: AsView + UpbGetArena + UpbGetMessagePtr,
    Self::Proxied: AssociatedMiniTable,
    for<'a> View<'a, Self::Proxied>: UpbGetMessagePtr,
{
    fn copy_from(&mut self, src: impl AsView<Proxied = Self::Proxied>) {
        // SAFETY: self and src are both valid `T`s associated with
        // `Self::mini_table()`.
        unsafe {
            assert!(upb_Message_DeepCopy(
                self.get_ptr(Private).raw(),
                src.as_view().get_ptr(Private).raw(),
                <Self::Proxied as AssociatedMiniTable>::mini_table(),
                self.get_arena(Private).raw()
            ));
        }
    }
}

impl<T> MergeFrom for T
where
    Self: AsView + UpbGetArena + UpbGetMessagePtr,
    Self::Proxied: AssociatedMiniTable,
    for<'a> View<'a, Self::Proxied>: UpbGetMessagePtr,
{
    fn merge_from(&mut self, src: impl AsView<Proxied = Self::Proxied>) {
        // SAFETY: self and src are both valid `T`s.
        unsafe {
            assert!(upb_Message_MergeFrom(
                self.get_ptr(Private).raw(),
                src.as_view().get_ptr(Private).raw(),
                <Self::Proxied as AssociatedMiniTable>::mini_table(),
                generated_extension_registry().as_ptr(),
                self.get_arena(Private).raw()
            ));
        }
    }
}

/// # Safety
/// - The field at `index` must be a message field of type `T`.
pub unsafe fn message_set_sub_message<
    'msg,
    P: Message + AssociatedMiniTable,
    T: Message + UpbGetMessagePtrMut + UpbGetArena,
>(
    parent: MessageMutInner<'msg, P>,
    index: u32,
    val: impl IntoProxied<T>,
) {
    // The message and arena are dropped after the setter. The
    // memory remains allocated as we fuse the arena with the
    // parent message's arena.
    let mut child = val.into_proxied(Private);
    parent.arena.fuse(child.get_arena(Private));

    let child_ptr = child.get_ptr_mut(Private);
    unsafe {
        // SAFETY:
        // - `parent.ptr` is valid as it comes from a `MessageMutInner`.
        // - The caller guarantees that `index` refers to a valid message field of type `T`.
        // - The child's arena has been fused into the parent's arena above.
        parent.ptr.set_base_field_message_at_index(index, child_ptr);
    }
}

/// # Safety
/// - The field at `index` must be a string field.
pub unsafe fn message_set_string_field<'msg, P: Message + AssociatedMiniTable>(
    parent: MessageMutInner<'msg, P>,
    index: u32,
    val: impl IntoProxied<ProtoString>,
) {
    let s = val.into_proxied(Private);
    let (view, arena) = s.into_inner(Private).into_raw_parts();
    parent.arena().fuse(&arena);
    unsafe {
        // SAFETY:
        // - `parent.ptr` is valid as it comes from a `MessageMutInner`.
        // - The caller guarantees that `index` refers to a valid string field.
        // - The string's arena has been fused into the parent's arena above.
        parent.ptr.set_base_field_string_at_index(index, view);
    }
}

/// # Safety
/// - The field at `index` must be a bytes field.
pub unsafe fn message_set_bytes_field<'msg, P: Message + AssociatedMiniTable>(
    parent: MessageMutInner<'msg, P>,
    index: u32,
    val: impl IntoProxied<ProtoBytes>,
) {
    let s = val.into_proxied(Private);
    let (view, arena) = s.into_inner(Private).into_raw_parts();
    parent.arena().fuse(&arena);
    unsafe {
        // SAFETY:
        // - `parent.ptr` is valid as it comes from a `MessageMutInner`.
        // - The caller guarantees that `index` refers to a valid bytes field.
        // - The string's arena has been fused into the parent's arena above.
        parent.ptr.set_base_field_string_at_index(index, view);
    }
}

/// # Safety
/// - The field at `index` must be a repeated field of `T`.
pub unsafe fn message_set_repeated_field<'msg, P: Message + AssociatedMiniTable, T: Singular>(
    parent: MessageMutInner<'msg, P>,
    index: u32,
    val: impl IntoProxied<Repeated<T>>,
) {
    let child = val.into_proxied(Private);
    let inner = child.inner(Private);
    parent.arena().fuse(inner.arena());
    unsafe {
        // SAFETY:
        // - `parent.ptr` is valid as it comes from a `MessageMutInner`.
        // - The caller guarantees that `index` refers to a valid repeated field.
        // - The repeated field's arena has been fused into the parent's arena above.
        parent.ptr.set_array_at_index(index, inner.raw());
    }
}

/// # Safety
/// - The field at `index` must be a map field with key type `K` and value type `V`.
pub unsafe fn message_set_map_field<
    'msg,
    P: Message + AssociatedMiniTable,
    K: MapKey,
    V: MapValue,
>(
    parent: MessageMutInner<'msg, P>,
    index: u32,
    val: impl IntoProxied<Map<K, V>>,
) {
    let mut child = val.into_proxied(Private);
    let child_as_mut = child.as_mut();
    let mut inner = child_as_mut.inner(Private);

    parent.arena().fuse(inner.arena());
    unsafe {
        // SAFETY:
        // - `parent.ptr` is valid as it comes from a `MessageMutInner`.
        // - The caller guarantees that `index` refers to a valid map field.
        // - The map's arena has been fused into the parent's arena above.
        parent.ptr.set_map_at_index(index, inner.as_raw());
    }
}
