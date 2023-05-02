use crate::{executor::TxResponse, transaction::SignedTx};
use ethereum::AccessList;
use evm::Config;
use primitive_types::{H160, U256};

pub struct ExecutorContext<'a> {
    pub caller: Option<H160>,
    pub to: Option<H160>,
    pub value: U256,
    pub data: &'a [u8],
    pub gas_limit: u64,
    pub access_list: AccessList,
}

pub trait Executor {
    const CONFIG: Config = Config::london();

    fn call(&mut self, ctx: ExecutorContext, apply: bool) -> TxResponse;

    fn exec(&mut self, tx: &SignedTx) -> TxResponse;
}
