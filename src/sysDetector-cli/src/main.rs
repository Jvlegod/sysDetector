use tokio::{io::{AsyncReadExt, AsyncWriteExt}, net::UnixStream};
use log::{debug, LevelFilter};
use serde_json;
use anyhow::Result;
mod args;

use self::{
    args::{Arguments},
};

// const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";
const SOCKET_FILE_NAME: &str = "/home/jvle/Desktop/temp/sysDetector.sock";

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Arguments::new()?;

    println!("{:?}", args.subcommand);

    let socket_path = SOCKET_FILE_NAME;
    let mut stream = UnixStream::connect(socket_path).await?;

    let serialized_cmd = serde_json::to_string(&args.subcommand)?;
    let cmd_bytes = serialized_cmd.as_bytes();

    let cmd_len = cmd_bytes.len() as u32;
    stream.write_all(&cmd_len.to_be_bytes()).await?;
    stream.write_all(cmd_bytes).await?;

    let mut buf = [0; 4];
    stream.read_exact(&mut buf).await?;
    let response_code =  i32::from_be_bytes(buf);

    println!("Received response: {}", response_code);

    Ok(())
}
    