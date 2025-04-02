use log::debug;
use serde_json;
use anyhow::Result;
use crate::args::Command;
use std::{path, process};

const EBPF_PATH: &str = "/home/jvle/Desktop/temp/";
const PROC_NAME: &str = "proc";

pub struct Ebpf{
    pub path: String,
}

impl Ebpf {
    pub fn new() -> Result<Self> {
        Ok(Self{
            path: EBPF_PATH.to_string()
        })  
    }

    pub fn handle_command(&self, command: &[u8]) -> Result<String> {
        let recv_command: Command = serde_json::from_slice(command)?;
        debug!("{:?}", recv_command);
    
        let response = match recv_command {
            Command::PROC { identifier, opt } => {
                let full_path = format!("{}/{}", self.path, PROC_NAME);
                let output = process::Command::new(full_path)
                   .args(opt)
                   .output()?;
    
                if output.status.success() {
                    let output_str = String::from_utf8_lossy(&output.stdout);
                    format!("Received PROC command. eBPF output: {}", output_str)
                } else {
                    let error_str = String::from_utf8_lossy(&output.stderr);
                    format!("Error executing eBPF program for PROC: {}", error_str)
                }
            }
            Command::FS { identifier, opt } => {
                todo!()
            }
            Command::LIST { identifier, opt } => {
                todo!()
            }
        };
    
        Ok(response)
    }       
}