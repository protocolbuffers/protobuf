use super::*;
use crate::__internal::entity_tag::*;
use crate::__internal::runtime::_opaque_pointees::RawRepeatedFieldData;
use crate::__internal::{EntityType, Enum, MatcherEq, Private, SealedInternal};
use crate::extension::SingularType;
use crate::extension::{ExtClear, ExtGet, ExtGetMut, ExtHas, ExtSet};
use crate::{ExtensionId, ExtensionKind, IntoMut, IntoView, MessageViewInterop};

unsafe extern "C" {
    pub fn proto2_rust_Message_clear_extension(m: RawMessage, number: i32);
    pub fn proto2_rust_Message_has_extension(m: RawMessage, number: i32) -> bool;
    pub fn proto2_rust_Message_set_extension_bool(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: bool,
    );
    pub fn proto2_rust_Message_get_extension_bool(
        m: RawMessage,
        number: i32,
        default_value: bool,
    ) -> bool;
    pub fn proto2_rust_Message_set_extension_float(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: f32,
    );
    pub fn proto2_rust_Message_get_extension_float(
        m: RawMessage,
        number: i32,
        default_value: f32,
    ) -> f32;
    pub fn proto2_rust_Message_set_extension_double(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: f64,
    );
    pub fn proto2_rust_Message_get_extension_double(
        m: RawMessage,
        number: i32,
        default_value: f64,
    ) -> f64;
    pub fn proto2_rust_Message_set_extension_int32(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: i32,
    );
    pub fn proto2_rust_Message_get_extension_int32(
        m: RawMessage,
        number: i32,
        default_value: i32,
    ) -> i32;
    pub fn proto2_rust_Message_set_extension_int64(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: i64,
    );
    pub fn proto2_rust_Message_get_extension_int64(
        m: RawMessage,
        number: i32,
        default_value: i64,
    ) -> i64;
    pub fn proto2_rust_Message_set_extension_uint32(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: u32,
    );
    pub fn proto2_rust_Message_get_extension_uint32(
        m: RawMessage,
        number: i32,
        default_value: u32,
    ) -> u32;
    pub fn proto2_rust_Message_set_extension_uint64(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: u64,
    );
    pub fn proto2_rust_Message_get_extension_uint64(
        m: RawMessage,
        number: i32,
        default_value: u64,
    ) -> u64;
    pub fn proto2_rust_Message_set_extension_string(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: CppStdString,
    );
    pub fn proto2_rust_Message_get_extension_string(
        m: RawMessage,
        number: i32,
        default_value: PtrAndLen,
    ) -> PtrAndLen;
    pub fn proto2_rust_Message_get_extension_message(
        m: RawMessage,
        number: i32,
        default_instance: RawMessage,
    ) -> RawMessage;
    pub fn proto2_rust_Message_mutable_extension_message(
        m: RawMessage,
        number: i32,
        field_type: i32,
        default_instance: RawMessage,
    ) -> RawMessage;

}

macro_rules! strong_pointer_asm {
    ($ptr:expr) => {
        // This inline assembly takes the pointer as a register input.
        // Because the assembly block is empty, it generates no actual
        // instructions. However, the compiler must still honor the input
        // constraint, forcing it to load the address of the symbol into
        // a register, which in turn creates the desired relocation.
        //
        // This requires `unsafe` because we are using the `asm!` macro.
        unsafe {
            std::arch::asm!("/* {0} */", in(reg) ($ptr as *const ()));
        }
    };
}

#[doc(hidden)]
pub struct InnerExtensionId {
    field_type: i32,
    cpp_extension_id_thunk: unsafe extern "C" fn() -> *const c_void,
    cpp_get_extension_thunk: Option<unsafe extern "C" fn(*const c_void) -> *const c_void>,
    cpp_mutable_extension_thunk: Option<unsafe extern "C" fn(*mut c_void) -> *mut c_void>,
}

impl InnerExtensionId {
    pub const fn new(
        field_type: i32,
        cpp_extension_id_thunk: unsafe extern "C" fn() -> *const c_void,
    ) -> Self {
        Self {
            field_type,
            cpp_extension_id_thunk,
            cpp_get_extension_thunk: None,
            cpp_mutable_extension_thunk: None,
        }
    }

    pub const fn new_repeated(
        field_type: i32,
        cpp_extension_id_thunk: unsafe extern "C" fn() -> *const c_void,
        cpp_get_extension_thunk: unsafe extern "C" fn(*const c_void) -> *const c_void,
        cpp_mutable_extension_thunk: unsafe extern "C" fn(*mut c_void) -> *mut c_void,
    ) -> Self {
        Self {
            field_type,
            cpp_extension_id_thunk,
            cpp_get_extension_thunk: Some(cpp_get_extension_thunk),
            cpp_mutable_extension_thunk: Some(cpp_mutable_extension_thunk),
        }
    }

    pub fn cpp_extension_id_thunk(&self) -> unsafe extern "C" fn() -> *const c_void {
        self.cpp_extension_id_thunk
    }

    pub fn cpp_get_extension_thunk(
        &self,
    ) -> Option<unsafe extern "C" fn(*const c_void) -> *const c_void> {
        self.cpp_get_extension_thunk
    }

    pub fn cpp_mutable_extension_thunk(
        &self,
    ) -> Option<unsafe extern "C" fn(*mut c_void) -> *mut c_void> {
        self.cpp_mutable_extension_thunk
    }
}

impl<Msg: Message, Kind> ExtHas<Msg> for ExtensionId<Msg, Kind>
where
    for<'a> Msg::MessageView<'a>: CppGetRawMessage,
    Kind: ExtensionKind + SingularType + 'static,
{
    fn has(&self, msg: impl AsView<Proxied = Msg>) -> bool {
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        unsafe {
            proto2_rust_Message_has_extension(
                msg.as_view().get_raw_message(Private),
                self.number() as i32,
            )
        }
    }
}

macro_rules! impl_scalar_extension {
    ($($t:ty => [$get:ident, $set:ident]),* $(,)?) => {
        $(
            impl<Extendee: Message> ExtGet<Extendee, $t, PrimitiveTag>
            for ExtensionId<Extendee, $t>
            where
                for<'a> Extendee::MessageView<'a>: CppGetRawMessage,
            {
                fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, $t> {
                    let msg = msg.into_view();
                    strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
                    unsafe {
                        $get(
                            msg.get_raw_message(Private),
                            self.number() as i32,
                            self.default,
                        )
                    }
                }
            }

            impl<Msg: Message> ExtSet<Msg, $t, PrimitiveTag>
            for ExtensionId<Msg, $t>
            where
                for<'a> Msg::MessageMut<'a>: CppGetRawMessageMut + AsMut,
            {
                fn set(&self, mut msg: impl AsMut<MutProxied = Msg>, value: $t) {
                    strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
                    unsafe {
                        $set(
                            msg.as_mut().get_raw_message_mut(Private),
                            self.number() as i32,
                            self.inner.field_type,
                            value,
                        );
                    }
                }
            }
        )*
    };
}

impl_scalar_extension!(
    bool => [proto2_rust_Message_get_extension_bool, proto2_rust_Message_set_extension_bool],
    f32 => [proto2_rust_Message_get_extension_float, proto2_rust_Message_set_extension_float],
    f64 => [proto2_rust_Message_get_extension_double, proto2_rust_Message_set_extension_double],
    i32 => [proto2_rust_Message_get_extension_int32, proto2_rust_Message_set_extension_int32],
    i64 => [proto2_rust_Message_get_extension_int64, proto2_rust_Message_set_extension_int64],
    u32 => [proto2_rust_Message_get_extension_uint32, proto2_rust_Message_set_extension_uint32],
    u64 => [proto2_rust_Message_get_extension_uint64, proto2_rust_Message_set_extension_uint64],
);
impl<Extendee: Message> ExtGet<Extendee, ProtoString, PrimitiveTag>
    for ExtensionId<Extendee, ProtoString>
where
    for<'a> Extendee::MessageView<'a>: CppGetRawMessage,
{
    fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, ProtoString> {
        let msg = msg.into_view();
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        let ptr_len = unsafe {
            proto2_rust_Message_get_extension_string(
                msg.get_raw_message(Private),
                self.number() as i32,
                self.default.into(),
            )
        };
        unsafe { ProtoStr::from_utf8_unchecked(ptr_len.as_ref()) }
    }
}

impl<Extendee: Message> ExtGet<Extendee, ProtoBytes, PrimitiveTag>
    for ExtensionId<Extendee, ProtoBytes>
where
    for<'a> Extendee::MessageView<'a>: CppGetRawMessage,
{
    fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, ProtoBytes> {
        let msg = msg.into_view();
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        let ptr_len = unsafe {
            proto2_rust_Message_get_extension_string(
                msg.get_raw_message(Private),
                self.number() as i32,
                self.default.as_ref().into(),
            )
        };
        unsafe { ptr_len.as_ref() }
    }
}

impl<Msg, V> ExtSet<Msg, V, PrimitiveTag> for ExtensionId<Msg, ProtoString>
where
    Msg: Message,
    V: IntoProxied<ProtoString>,
    for<'a> Msg::MessageMut<'a>: CppGetRawMessageMut + AsMut,
{
    fn set(&self, mut msg: impl AsMut<MutProxied = Msg>, value: V) {
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        unsafe {
            proto2_rust_Message_set_extension_string(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                <ProtoString as CppTypeConversions>::into_insertelem(value.into_proxied(Private)),
            );
        }
    }
}

impl<Msg, V> ExtSet<Msg, V, PrimitiveTag> for ExtensionId<Msg, ProtoBytes>
where
    Msg: Message,
    V: IntoProxied<ProtoBytes>,
    for<'a> Msg::MessageMut<'a>: CppGetRawMessageMut + AsMut,
{
    fn set(&self, mut msg: impl AsMut<MutProxied = Msg>, value: V) {
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        unsafe {
            proto2_rust_Message_set_extension_string(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                <ProtoBytes as CppTypeConversions>::into_insertelem(value.into_proxied(Private)),
            );
        }
    }
}

impl<Extendee: Message, V> ExtGet<Extendee, V, MessageTag> for ExtensionId<Extendee, V>
where
    for<'a> Extendee::MessageView<'a>: CppGetRawMessage,
    V: Message + Singular + ExtensionKind + 'static,
    for<'b> <V as Proxied>::View<'b>: CppGetRawMessage + MessageViewInterop<'b> + Default,
{
    fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, V> {
        let msg = msg.into_view();
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        let default_instance = <V as Proxied>::View::default();
        let raw_msg = unsafe {
            proto2_rust_Message_get_extension_message(
                msg.get_raw_message(Private),
                self.number() as i32,
                default_instance.get_raw_message(Private),
            )
        };
        unsafe {
            <View<'msg, V>>::__unstable_wrap_raw_message_unchecked_lifetime(
                raw_msg.as_ptr() as *const std::ffi::c_void
            )
        }
    }
}

impl<Extendee, V> ExtGetMut<Extendee, V, MessageTag> for ExtensionId<Extendee, V>
where
    Extendee: Message,
    V: Message,
    for<'a> Extendee::MessageMut<'a>: CppGetRawMessageMut,
    V: Message + Singular + ExtensionKind + 'static,
    for<'b> <V as MutProxied>::Mut<'b>: CppGetRawMessageMut + MessageMutInterop<'b>,
    for<'b> <V as Proxied>::View<'b>: CppGetRawMessage + Default,
{
    fn get_mut<'msg>(&self, msg: impl IntoMut<'msg, MutProxied = Extendee>) -> Mut<'msg, V> {
        let mut msg = msg.into_mut();
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        let default_instance = <V as Proxied>::View::default();
        let raw_msg = unsafe {
            proto2_rust_Message_mutable_extension_message(
                msg.get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                default_instance.get_raw_message(Private),
            )
        };
        unsafe {
            <Mut<'msg, V>>::__unstable_wrap_raw_message_mut_unchecked_lifetime(
                raw_msg.as_ptr() as *mut std::ffi::c_void
            )
        }
    }
}

impl<Msg: Message, Kind> ExtClear<Msg> for ExtensionId<Msg, Kind>
where
    for<'a> Msg::MessageMut<'a>: CppGetRawMessageMut,
    Kind: ExtensionKind + 'static,
{
    // TODO: should this support repeated extensions?
    fn clear(&self, mut msg: impl AsMut<MutProxied = Msg>) {
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        unsafe {
            proto2_rust_Message_clear_extension(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
            )
        }
    }
}

impl<Msg, V, VProxied> ExtSet<Msg, VProxied, MessageTag> for ExtensionId<Msg, V>
where
    Msg: Message,
    V: Message + Singular + ExtensionKind + 'static,
    VProxied: IntoProxied<V>,
    for<'a> Msg::MessageMut<'a>: CppGetRawMessageMut,
    for<'a> <V as MutProxied>::Mut<'a>: CppGetRawMessageMut + MessageMutInterop<'a>,
    for<'a> <V as Proxied>::View<'a>: CppGetRawMessage + Default,
{
    fn set(&self, mut msg: impl AsMut<MutProxied = Msg>, value: VProxied) {
        let value = value.into_proxied(Private);
        let mut ext_mut = self.get_mut(msg.as_mut());
        ext_mut.take_from(value);
    }
}

impl<Msg, V> ExtGet<Msg, Repeated<V>, RepeatedTag> for ExtensionId<Msg, Repeated<V>>
where
    Msg: Message,
    V: Singular,
    Repeated<V>: ExtensionKind,
    for<'a> Msg::MessageView<'a>: CppGetRawMessage,
{
    fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Msg>) -> View<'msg, Repeated<V>> {
        let msg = msg.into_view();
        let raw = unsafe {
            let thunk = self.inner.cpp_get_extension_thunk().unwrap();
            thunk(msg.get_raw_message(Private).as_ptr() as *const std::ffi::c_void)
                as *mut RawRepeatedFieldData
        };
        unsafe {
            IntoView::into_view(RepeatedView::from_raw(
                Private,
                RawRepeatedField::new(raw).unwrap(),
            ))
        }
    }
}

impl<Msg, V> ExtGetMut<Msg, Repeated<V>, RepeatedTag> for ExtensionId<Msg, Repeated<V>>
where
    Msg: Message,
    V: Singular,
    Repeated<V>: ExtensionKind,
    for<'a> Msg::MessageMut<'a>: CppGetRawMessageMut,
{
    fn get_mut<'msg>(&self, msg: impl IntoMut<'msg, MutProxied = Msg>) -> Mut<'msg, Repeated<V>> {
        let mut msg = msg.into_mut();
        let raw = unsafe {
            let thunk = self.inner.cpp_mutable_extension_thunk().unwrap();
            thunk(msg.get_raw_message_mut(Private).as_ptr() as *mut std::ffi::c_void)
                as *mut RawRepeatedFieldData
        };
        unsafe {
            RepeatedMut::from_inner(
                Private,
                InnerRepeatedMut::new(RawRepeatedField::new(raw).unwrap()),
            )
            .into_mut()
        }
    }
}

impl<Msg, V, VProxied> ExtSet<Msg, VProxied, RepeatedTag> for ExtensionId<Msg, Repeated<V>>
where
    Msg: Message,
    V: Singular,
    Repeated<V>: ExtensionKind,
    for<'a> Msg::MessageMut<'a>: CppGetRawMessageMut,
    VProxied: IntoProxied<Repeated<V>>,
{
    fn set(&self, mut msg: impl AsMut<MutProxied = Msg>, value: VProxied) {
        let value = value.into_proxied(Private);
        let mut ext_mut = self.get_mut(msg.as_mut());
        ext_mut.clear();
        ext_mut.copy_from(unsafe { RepeatedView::from_raw(Private, value.inner(Private).raw()) });
    }
}

impl<
        Extendee: Message,
        E: Enum
            + Copy
            + 'static
            + SealedInternal
            + for<'a> Proxied<View<'a> = E>
            + EntityType<Tag = EnumTag>,
    > ExtGet<Extendee, E, EnumTag> for ExtensionId<Extendee, E>
where
    for<'a> Extendee::MessageView<'a>: CppGetRawMessage,
    i32: From<<E as ExtensionKind>::DefaultType>,
{
    fn get<'msg>(&self, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, E> {
        let msg = msg.into_view();
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        let val = unsafe {
            proto2_rust_Message_get_extension_int32(
                msg.get_raw_message(Private),
                self.number() as i32,
                <E as ExtensionKind>::DefaultType::from(self.default).into(),
            )
        };
        E::try_from(val).unwrap_or(self.default)
    }
}

impl<
        Msg: Message,
        E: Enum
            + Copy
            + 'static
            + SealedInternal
            + for<'a> Proxied<View<'a> = E>
            + EntityType<Tag = EnumTag>,
    > ExtSet<Msg, E, EnumTag> for ExtensionId<Msg, E>
where
    for<'a> Msg::MessageMut<'a>: CppGetRawMessage,
    <E as ExtensionKind>::DefaultType: From<E>,
    i32: From<<E as ExtensionKind>::DefaultType>,
{
    fn set(&self, mut msg: impl AsMut<MutProxied = Msg>, value: E) {
        strong_pointer_asm!(self.inner.cpp_extension_id_thunk());
        unsafe {
            proto2_rust_Message_set_extension_int32(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                <E as ExtensionKind>::DefaultType::from(value).into(),
            );
        }
    }
}
