use std::ffi::CStr;
use serde::{ser::SerializeStruct, Deserialize, Serialize};

use super::bindings;

macro_rules! serialize_c_string {
    ($state:expr, $field_name:expr, $c_string_ptr:expr) => {
        if !$c_string_ptr.is_null() {
            $state.serialize_field($field_name, unsafe {
                CStr::from_ptr($c_string_ptr).to_string_lossy().as_ref()
            })?;
        }
    };
}

impl Serialize for bindings::extra_descr_data {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let mut state = serializer.serialize_struct("extra_descr_data", 2)?;
        serialize_c_string!(state, "keyword", self.keyword);
        serialize_c_string!(state, "description", self.description);
        state.end()
    }
}

impl<'de> Deserialize<'de> for bindings::extra_descr_data {
    fn deserialize<D>(deserializer: D) -> Result<bindings::extra_descr_data, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        #[derive(Deserialize)]
        struct ExtraDescrData {
            keyword: Option<String>,
            description: Option<String>,
        }

        let extra_descr_data = ExtraDescrData::deserialize(deserializer)?;
        Ok(bindings::extra_descr_data {
            keyword: extra_descr_data.keyword.map(|s| std::ffi::CString::new(s).unwrap().into_raw()).unwrap_or(std::ptr::null_mut()),
            description: extra_descr_data.description.map(|s| std::ffi::CString::new(s).unwrap().into_raw()).unwrap_or(std::ptr::null_mut()),
            ..Default::default()
        })
    }
}