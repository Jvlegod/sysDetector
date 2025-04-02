use log::{debug, LevelFilter};
use clap::{Parser, Subcommand};
use anyhow::Result;
use serde::{Serialize, Deserialize};

const DEFAULT_LOG_LEVEL: &str = "info";

#[derive(Debug, Clone, Subcommand, Serialize, Deserialize)]
pub enum Command {
    /// Set proc op
    PROC {
        identifier: String,

        /// Set proc opt
        #[clap(required = true)]
        opt: Vec<String>,
    },

    /// Set fs op
    FS {
        identifier: String,

        /// Set fs opt
        #[clap(required = true)]
        opt: Vec<String>,
    },

    /// Set list op
    LIST {
        identifier: String,

        /// Set list opt
        #[clap(required = true)]
        opt: Vec<String>,
    },
}

#[derive(Debug, Clone, Parser)]
pub struct Arguments {
    /// Set the subcommand
    #[clap(subcommand)]
    pub subcommand: Command,

    /// Set the logging level ("trace"|"debug"|"info"|"warn"|"error")
    #[clap(short, long, default_value = DEFAULT_LOG_LEVEL)]
    pub log_level: String,
}

impl Arguments {
    pub fn new() -> Result<Self> {
        let mut args = Self::parse();
        match &mut args.subcommand {
            Command::PROC { ref mut identifier, .. } => {
                *identifier = "proc".to_string();
            }
            Command::FS { ref mut identifier, .. } => {
                *identifier = "fs".to_string();
            }
            Command::LIST { ref mut identifier, .. } => {
                *identifier = "list".to_string();
            }
        }
        Ok(args)
    }

    pub fn get_log_level(&self) -> Result<LevelFilter> {
        match self.log_level.to_lowercase().as_str() {
            "trace" => Ok(LevelFilter::Trace),
            "debug" => Ok(LevelFilter::Debug),
            "info" => Ok(LevelFilter::Info),
            "warn" => Ok(LevelFilter::Warn),
            "error" => Ok(LevelFilter::Error),
            _ => Err(anyhow::anyhow!("Invalid log level: {}", self.log_level)),
        }
    }
}