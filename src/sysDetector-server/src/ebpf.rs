use std::process::Command;
use serde_json;
use anyhow::Result;
mod args;

pub fn handle_command(command: &[u8]) -> Result<String> {
    let recv_command: Command = serde_json::from_slice(command)?;
    println!("{:?}", recv_command);

    let response = match recv_command {
        Command::PROC { identifier, opt } => {
            // execute proc
            let output = Command::new("./ebpf_program")
               .arg("PROC")
               .arg(identifier)
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
        Command::FS { identifiers, opt } => {
            // execute fs
            let output = Command::new("./ebpf_program")
               .arg("FS")
               .args(identifiers)
               .args(opt)
               .output()?;

            if output.status.success() {
                let output_str = String::from_utf8_lossy(&output.stdout);
                format!("Received FS command. eBPF output: {}", output_str)
            } else {
                let error_str = String::from_utf8_lossy(&output.stderr);
                format!("Error executing eBPF program for FS: {}", error_str)
            }
        }
        Command::LIST { identifiers, opt } => {
            // execute list
            let output = Command::new("./ebpf_program")
               .arg("LIST")
               .args(identifiers)
               .args(opt)
               .output()?;

            if output.status.success() {
                let output_str = String::from_utf8_lossy(&output.stdout);
                format!("Received LIST command. eBPF output: {}", output_str)
            } else {
                let error_str = String::from_utf8_lossy(&output.stderr);
                format!("Error executing eBPF program for LIST: {}", error_str)
            }
        }
    };

    Ok(response)
}
    