use log::debug;
use serde_json;
use anyhow::{Context, Result};
use crate::args::Command;
use mqueue::{Mqueue, QueueOptions, QueueAttr};
use std::time::Duration;

const EBPF_PATH: &str = "/home/jvle/Desktop/temp";
const COMMAND_QUEUE: &str = "/ebpf_command_queue";
const RESPONSE_QUEUE: &str = "/ebpf_response_queue";
const MAX_MSG_SIZE: usize = 1024;
const QUEUE_TIMEOUT: Duration = Duration::from_secs(5);

#[repr(i32)]
pub enum RPC_RET_CODE {
    EBPF_FAILED,
    EBPF_SUCCESS,
}

impl RPC_RET_CODE {
    pub fn get_code(&self) -> i32 {
        match self {
            Self::EBPF_FAILED => -1,
            Self::EBPF_SUCCESS => 0,
        }
    }
}

pub struct Rpc {
    pub path: String,
}

impl Rpc {
    pub fn new() -> Result<Self> {
        Ok(Self {
            path: EBPF_PATH.to_string()
        })
    }

    pub fn handle_command(&self, command: &[u8]) -> Result<i32> {
        let recv_command: Command = serde_json::from_slice(command)
            .context("Failed to deserialize command")?;
        debug!("Received command: {:?}", recv_command);

        let cmd_mq = Mqueue::open(
            COMMAND_QUEUE,
            QueueOptions::new()
                .write_only()
                .create()
                .nonblock(),
            QueueAttr {
                maxmsg: 10,
                msgsize: MAX_MSG_SIZE as u64,
                ..Default::default()
            },
        ).context("Failed to open command queue")?;

        let command_str = self.validate_command(&recv_command)?;

        cmd_mq.send(command_str.as_bytes(), 0)
            .context("Failed to send command")?;
        debug!("Sent command: {}", command_str);

        let resp_mq = Mqueue::open(
            RESPONSE_QUEUE,
            QueueOptions::new()
                .read_only()
                .create(),
            QueueAttr {
                maxmsg: 10,
                msgsize: MAX_MSG_SIZE as u64,
                ..Default::default()
            },
        ).context("Failed to open response queue")?;

        let mut buffer = [0u8; MAX_MSG_SIZE];
        let (msg, _) = resp_mq.receive_timeout(&mut buffer, QUEUE_TIMEOUT)
            .context("Timeout waiting for response")?;

        let response = String::from_utf8_lossy(&msg);
        response.trim().parse::<i32>()
            .context("Failed to parse response code")
    }

    fn validate_command(&self, cmd: &Command) -> Result<String> {
        match cmd {
            Command::PROC { identifier, opt } => {
                if opt.iter().any(|s| s == "start") {
                    Ok(format!("PROC_START:{}", identifier))
                } else if opt.iter().any(|s| s == "stop") {
                    Ok(format!("PROC_STOP:{}", identifier))
                } else {
                    Err(anyhow::anyhow!("Invalid PROC operation"))
                }
            }
            Command::FS { identifier, opt } => {
                Ok(format!("FS_OP:{}:{}", identifier, opt.join(",")))
            }
            Command::LIST { identifier, opt } => {
                Ok(format!("LIST:{}:{}", identifier, opt.join(",")))
            }
        }
    }
}