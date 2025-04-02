use core::str;
use std::path::Path;
use log::info;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::UnixListener,
};

mod ebpf;
mod args;


use anyhow::Result;

// const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";
const SOCKET_FILE_NAME: &str = "/home/jvle/Desktop/temp/sysDetector.sock";

async fn handle_connection(mut stream: tokio::net::UnixStream) -> Result<()> {
    let ebpf = ebpf::Ebpf::new()?;
    let mut len_buf = [0; 4];
    stream.read_exact(&mut len_buf).await?;
    let message_len = u32::from_be_bytes(len_buf) as usize;

    let mut buf = vec![0; message_len];
    stream.read_exact(&mut buf).await?;

    let response = ebpf.handle_command(&buf)?;

    let response_len = response.len() as u32;
    stream.write_all(&response_len.to_be_bytes()).await?;
    stream.write_all(response.as_bytes()).await?;

    Ok(())
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
    