use std::path::Path;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::UnixListener,
};

// const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";
const SOCKET_FILE_NAME: &str = "/home/jvle/Desktop/temp/sysDetector.sock";

#[repr(u8)]
enum DeamonCommand {
    START = 1,
    STOP = 2,
    LIST = 3,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    if Path::new(SOCKET_FILE_NAME).exists() {
        std::fs::remove_file(SOCKET_FILE_NAME)?;
    }

    let listener = UnixListener::bind(SOCKET_FILE_NAME)?;
    println!("sysDetector server listening on {}", SOCKET_FILE_NAME);

    loop {
        let (mut stream, _) = listener.accept().await?;
        println!("New client connected");

        tokio::spawn(async move {
            let mut buf = [0u8; 1];
            if let Ok(n) = stream.read(&mut buf).await {
                if n > 0 {
                    let command_code = buf[0];
                    let response = match command_code {
                        code if code == DeamonCommand::START as u8 => {
                            "Service started".to_string()
                        }
                        code if code == DeamonCommand::STOP as u8 => {
                            "Service stopped".to_string()
                        }
                        code if code == DeamonCommand::LIST as u8 => {
                            "Listing services...".to_string()
                        }
                        _ => {
                            "Unknown command".to_string()
                        }
                    };
                    println!("{:?}", response);
                    if let Err(e) = stream.write_all(response.as_bytes()).await {
                        eprintln!("Error sending response: {}", e);
                    }
                }
            }
        });
    }
}    