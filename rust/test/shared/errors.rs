use std::fmt;
use std::fmt::Debug;

#[derive(Debug, PartialEq, Clone)]
pub struct DecodeError {
    pub cause: String,
}

impl DecodeError {
    pub fn new(msg: &str) -> Self {
        Self { cause: msg.to_string() }
    }
}

impl fmt::Display for DecodeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Decode Error because of {}", self.cause)
    }
}
