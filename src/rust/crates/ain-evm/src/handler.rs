use crate::block::BlockHandler;
use crate::evm::{get_vicinity, EVMHandler};
use crate::executor::AinExecutor;
use crate::traits::Executor;
use ethereum::{Block, PartialHeader, TransactionV2};
use evm::backend::{MemoryBackend, MemoryVicinity};
use primitive_types::{H160, H256, U256};
use std::error::Error;
use std::time::{SystemTime, UNIX_EPOCH};
pub struct Handlers {
    pub evm: EVMHandler,
    pub block: BlockHandler,
}

impl Handlers {
    pub fn new() -> Self {
        Self {
            evm: EVMHandler::new(),
            block: BlockHandler::new(),
        }
    }

    pub fn finalize_block(
        &self,
        context: u64,
        update_state: bool,
    ) -> Result<(Block<TransactionV2>, Vec<TransactionV2>), Box<dyn Error>> {
        let mut transactions = Vec::with_capacity(self.evm.tx_queues.len(context));
        let mut failed_transactions = Vec::with_capacity(self.evm.tx_queues.len(context));
        let vicinity = get_vicinity(None, None);
        let state = self.evm.tx_queues.state(context).expect("Wrong context");
        let backend = MemoryBackend::new(&vicinity, state);
        let mut executor = AinExecutor::new(backend);

        for signed_tx in self.evm.tx_queues.drain_all(context) {
            let tx_response = executor.exec(&signed_tx);
            if tx_response.exit_reason.is_succeed() {
                transactions.push(signed_tx.transaction);
            } else {
                failed_transactions.push(signed_tx.transaction)
            }
        }

        let mut all_transactions: Vec<TransactionV2> = Vec::new();
        all_transactions.extend(transactions.clone());
        all_transactions.extend(failed_transactions.clone());

        self.evm.tx_queues.remove(context);

        if update_state {
            let mut state = self.evm.state.write().unwrap();
            *state = executor.backend().state().clone();
        }

        let (parent_hash, number) = {
            let blocks = self.block.blocks.read().unwrap();
            blocks
                .first()
                .and_then(|first_block| Some((first_block.header.hash(), blocks.len() + 1)))
                .unwrap_or((H256::default(), 0))
        };

        let block = Block::new(
            PartialHeader {
                parent_hash,
                beneficiary: Default::default(),
                state_root: Default::default(),
                receipts_root: Default::default(),
                logs_bloom: Default::default(),
                difficulty: Default::default(),
                number: U256::from(number),
                gas_limit: Default::default(),
                gas_used: Default::default(),
                timestamp: SystemTime::now()
                    .duration_since(UNIX_EPOCH)
                    .unwrap()
                    .as_millis() as u64,
                extra_data: Default::default(),
                mix_hash: Default::default(),
                nonce: Default::default(),
            },
            all_transactions,
            Vec::new(),
        );

        self.block.connect_block(block.clone());

        Ok((block, failed_transactions))
    }
}
