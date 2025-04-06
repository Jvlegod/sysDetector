/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Author: Keke Ming
 * Date: 20250405
 */
use log::debug;
use serde_json;
use anyhow::{Context, Result};
use nix::mqueue::{mq_open, mq_send, mq_receive, mq_close, mq_unlink, MqAttr, MQ_OFlag, MqdT};
use nix::sys::stat::Mode;
use std::ffi::CStr;
use std::time::Duration;
use crate::args::Command;
use lazy_static::lazy_static;

use crate::args;

lazy_static! {
    static ref COMMAND_QUEUE: &'static CStr = {
        CStr::from_bytes_with_nul(b"/ebpf_command_queue\0").unwrap()
    };
    static ref RESPONSE_QUEUE: &'static CStr = {
        CStr::from_bytes_with_nul(b"/ebpf_response_queue\0").unwrap()
    };
}

const MAX_MSG_SIZE: usize = 1024;
// TODO: set the attr for mqueue, for example: MqAttr.
const QUEUE_TIMEOUT: Duration = Duration::from_secs(5);

#[repr(i32)]
#[derive(Debug)]
pub enum RpcRetCode {
    Success = 0,
    Failed = -1,
}

impl RpcRetCode {
    pub fn get_code(&self) -> i32 {
        match self {
            Self::Success => -1,
            Self::Failed => 0,
        }
    }
}

pub struct Rpc {
    cmd_fd: Option<MqdT>,
    resp_fd: Option<MqdT>,
    // TODO: now we dont't need it.
    is_owner: bool,
}

impl Rpc {
    pub fn new() -> Result<Self> {
        let attr = MqAttr::new (
            0,
            10,
            MAX_MSG_SIZE as i64,
            0,
        );

        Ok(Self {
            cmd_fd: Some(mq_open(
                *COMMAND_QUEUE,
                MQ_OFlag::O_CREAT | MQ_OFlag::O_WRONLY,
                Mode::S_IRUSR | Mode::S_IWUSR,
                Some(&attr),
            ).context("Failed to create command queue")?),
            
            resp_fd: Some(mq_open(
                *RESPONSE_QUEUE,
                MQ_OFlag::O_CREAT | MQ_OFlag::O_RDONLY,
                Mode::S_IRUSR | Mode::S_IWUSR,
                Some(&attr),
            ).context("Failed to create response queue")?),
            
            is_owner: true,
        })
    }

    pub fn handle_command(&self, command: &[u8]) -> Result<i32> {
        let recv_command: Command = serde_json::from_slice(command)
        .context("Failed to deserialize command")?;
        println!("Received command: {:?}", recv_command);

        let command_str = self.validate_command(&recv_command)?;
        self.send_command(&command_str)?;
        println!("Send command: {}", command_str);
        
        let recv_str = self.receive_response()?;
        println!("Received response: {}", recv_str);
        /* TODO:
         * Here now always success, but need to return related value
         */
        Ok(RpcRetCode::Success.get_code())
    }

    fn send_command(&self, command: &str) -> Result<()> {
        let bytes = command.as_bytes();
        if bytes.len() > MAX_MSG_SIZE {
            return Err(anyhow::anyhow!("Message too large"));
        }
        
        self.cmd_fd.as_ref().ok_or(anyhow::anyhow!("Command queue closed"))?;
        
        mq_send(self.cmd_fd.as_ref().unwrap(), bytes, 0)
            .context("Command send failed")
    }

    fn receive_response(&self) -> Result<String> {
        let mut buf = vec![0u8; MAX_MSG_SIZE];
        let mut prio = 0;
        
        self.resp_fd.as_ref().ok_or(anyhow::anyhow!("Response queue closed"))?;
        
        let len = match mq_receive(self.resp_fd.as_ref().unwrap(),
        &mut buf, &mut prio) {
            Ok(len) => len,
            Err(err) => {
                eprintln!("Error receiving response: {}", err);
                return Err(err).context("Response receive failed");
            }
        };

        String::from_utf8(buf[..len].to_vec())
            .context("Invalid UTF-8 response")
    }

    fn validate_command(&self, cmd: &Command) -> Result<String> {
        match cmd {
            Command::PROC { opt, identifier } => {
                if args::PROC_POSSIBLE_OPT_VALUES.contains(&opt.as_str()) {
                    let mut result = opt.to_string();
                    if !identifier.is_empty() {
                        result.push_str(" ");
                        result.push_str(&identifier.join(" "));
                    }
                    Ok(result)
                } else {
                    Err(anyhow::anyhow!("Invalid PROC operation"))
                }
            }
            Command::FS { identifier, opt } => {
                todo!()
            }
            Command::LIST { identifier, opt } => {
                todo!()
            }
        }
    }
}

impl Drop for Rpc {
    fn drop(&mut self) {
        let close = |fd: &mut Option<MqdT>| {
            if let Some(fd) = fd.take() {
                if let Err(e) = mq_close(fd) {
                    eprintln!("Error closing queue: {}", e);
                }
            }
        };

        close(&mut self.cmd_fd);
        close(&mut self.resp_fd);

        if self.is_owner {
            let unlink = |name: &CStr| {
                if let Err(e) = mq_unlink(name) {
                    eprintln!("Error unlinking queue: {}", e);
                }
            };

            unlink(*COMMAND_QUEUE);
            unlink(*RESPONSE_QUEUE);
        }
    }
}