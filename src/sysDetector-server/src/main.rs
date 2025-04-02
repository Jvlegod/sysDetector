use core::str;
use std::path::Path;
use log::info;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::UnixListener,
};
use anyhow::Result;

mod ebpf;
mod args;

use self::{
    args::{Command},
};

// const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";
const SOCKET_FILE_NAME: &str = "/home/jvle/Desktop/temp/sysDetector.sock";

pub fn handle_command(command: &[u8]) -> Result<String> {
    let recv_command: Command = serde_json::from_slice(command)?;
    println!("{:?}", recv_command);
    let response = match recv_command {
        Command::PROC { identifier, opt } => {
            format!("Received PROC command with identifier: {}, opt: {:?}", identifier, opt)
        }
        Command::FS { identifier, opt } => {
            format!("Received FS command with identifiers: {:?}, opt: {:?}", identifier, opt)
        }
        Command::LIST { identifier, opt } => {
            format!("Received LIST command with identifiers: {:?}, opt: {:?}", identifier, opt)
        }
    };
    Ok(response)
}

async fn handle_connection(mut stream: tokio::net::UnixStream) {
    // read length
    let mut len_buf = [0; 4];
    if let Err(e) = stream.read_exact(&mut len_buf).await {
        eprintln!("Error reading message length: {}", e);
        return;
    }
    let message_len = u32::from_be_bytes(len_buf) as usize;

    // read content
    let mut buf = vec![0; message_len];
    if let Err(e) = stream.read_exact(&mut buf).await {
        eprintln!("Error reading message: {}", e);
        return;
    }

    let response = match handle_command(&buf) {
        Ok(resp) => resp,
        Err(e) => {
            eprintln!("Error handling command: {}", e);
            "Error handling command".to_string()
        }
    };

    // send length
    let response_len = response.len() as u32;
    if let Err(e) = stream.write_all(&response_len.to_be_bytes()).await {
        eprintln!("Error sending response length: {}", e);
        return;
    }

    // send content
    if let Err(e) = stream.write_all(response.as_bytes()).await {
        eprintln!("Error sending response: {}", e);
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    if Path::new(SOCKET_FILE_NAME).exists() {
        std::fs::remove_file(SOCKET_FILE_NAME)?;
    }

    let listener = UnixListener::bind(SOCKET_FILE_NAME)?;
    info!("sysDetector server listening on {}", SOCKET_FILE_NAME);

    loop {
        let (stream, _) = listener.accept().await?;
        tokio::spawn(handle_connection(stream));
    }
}
    