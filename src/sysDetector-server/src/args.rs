use log::{debug, LevelFilter};
use clap::Subcommand;
use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Subcommand, Serialize, Deserialize)]
pub enum Command {
    /// Set proc op
    PROC {
        identifier: String,

        /// Set proc opt
        #[clap(required = true)]
        opt: String,
    },

    /// Set fs op
    FS {
        identifier: String,

        /// Set fs opt
        #[clap(required = true)]
        opt: String,
    },

    /// Set list op
    LIST {
        identifier: String,

        /// Set list opt
        #[clap(required = true)]
        opt: String,
    },
}