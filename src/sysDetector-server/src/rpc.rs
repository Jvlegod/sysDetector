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
use serde_json;
use anyhow::{Context, Result};
use nix::mqueue::{mq_open, mq_send, mq_receive, mq_close, MqAttr, MQ_OFlag, MqdT};
use nix::sys::stat::Mode;
use std::ffi::CStr;
use std::fs;
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
    static ref FS_COMMAND_QUEUE: &'static CStr = {
        CStr::from_bytes_with_nul(b"/fs_command_queue\0").unwrap()
    };
    static ref FS_RESPONSE_QUEUE: &'static CStr = {
        CStr::from_bytes_with_nul(b"/fs_response_queue\0").unwrap()
    };
}

const MAX_MSG_SIZE: usize = 1024;
const OUT_FILE_NAME: &str = "/var/log/sysDetector/out.log";

pub struct RpcResponse {
    pub code: i32,
    pub body: String,
}

pub struct Rpc;

impl Rpc {
    pub fn new() -> Result<Self> {
        Ok(Self)
    }

    pub fn handle_command(&self, command: &[u8]) -> Result<RpcResponse> {
        let recv_command: Command = serde_json::from_slice(command)
        .context("Failed to deserialize command")?;
        println!("Received command: {:?}", recv_command);

        let command_str = self.validate_command(&recv_command)?;
        let (command_queue, response_queue) = Self::queues_for_command(&recv_command);
        let response_fd = Self::open_response_queue(response_queue)?;
        self.send_command_to(&command_str, command_queue)?;
        println!("Send command: {}", command_str);
        
        let recv_str = Self::receive_response_from_fd(response_fd)?;
        println!("Received response: {}", recv_str);
        let code = recv_str.trim().parse::<i32>().unwrap_or(1);
        let body = if Self::is_list_command(&recv_command) {
            fs::read_to_string(OUT_FILE_NAME).unwrap_or_default()
        } else {
            String::new()
        };

        Ok(RpcResponse { code, body })
    }

    fn is_list_command(cmd: &Command) -> bool {
        matches!(cmd, Command::PROC { opt, .. } | Command::FS { opt, .. } if opt == "list")
    }

    fn queues_for_command(cmd: &Command) -> (&'static CStr, &'static CStr) {
        match cmd {
            Command::FS { .. } => (*FS_COMMAND_QUEUE, *FS_RESPONSE_QUEUE),
            _ => (*COMMAND_QUEUE, *RESPONSE_QUEUE),
        }
    }

    fn open_response_queue(queue_name: &CStr) -> Result<MqdT> {
        let attr = MqAttr::new(0, 10, MAX_MSG_SIZE as i64, 0);
        mq_open(
            queue_name,
            MQ_OFlag::O_CREAT | MQ_OFlag::O_RDONLY,
            Mode::S_IRUSR | Mode::S_IWUSR,
            Some(&attr),
        ).context("Response queue open failed")
    }

    fn receive_response_from_fd(fd: MqdT) -> Result<String> {
        let mut buf = vec![0u8; MAX_MSG_SIZE];
        let mut prio = 0;
        let len = mq_receive(&fd, &mut buf, &mut prio)
            .context("Response receive failed")?;
        mq_close(fd).ok();

        String::from_utf8(buf[..len].to_vec())
            .context("Invalid UTF-8 response")
    }

    fn send_command_to(&self, command: &str, queue_name: &CStr) -> Result<()> {
        let bytes = command.as_bytes();
        if bytes.len() > MAX_MSG_SIZE {
            return Err(anyhow::anyhow!("Message too large"));
        }

        let fd = mq_open(
            queue_name,
            MQ_OFlag::O_WRONLY,
            Mode::S_IRUSR | Mode::S_IWUSR,
            None,
        ).context("Command send queue open failed")?;

        let result = mq_send(&fd, bytes, 0).context("Command send failed");
        mq_close(fd).ok();
        result
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
            Command::FS { opt, identifier } => {
                if args::FS_POSSIBLE_OPT_VALUES.contains(&opt.as_str()) {
                    let mut result = opt.to_string();
                    if !identifier.is_empty() {
                        result.push_str(" ");
                        result.push_str(&identifier.join(" "));
                    }
                    Ok(result)
                } else {
                    Err(anyhow::anyhow!("Invalid FS operation"))
                }
            }
            Command::LIST { .. } => {
                Err(anyhow::anyhow!("LIST operation is not implemented"))
            }
        }
    }
}
