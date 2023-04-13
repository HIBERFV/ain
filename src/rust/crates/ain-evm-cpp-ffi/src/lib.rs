use std::error::Error;

// #[cxx::bridge]
// mod ffi {
//     unsafe extern "C++" {
//         include!("masternodes/evm_ffi.h");

//         fn getChainId() -> u64;
//         fn isMining() -> bool;
//     }
// }

pub fn get_chain_id() -> Result<u64, Box<dyn Error>> {
    // let chain_id = ffi::getChainId();
    Ok(0)
}

pub fn is_mining() -> Result<bool, Box<dyn Error>> {
    // let is_mining = ffi::isMining();
    Ok(true)
}