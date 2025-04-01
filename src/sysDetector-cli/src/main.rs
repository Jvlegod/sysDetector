use structopt::StructOpt;
use tokio::{io::{AsyncReadExt, AsyncWriteExt}, net::UnixStream};

// const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";
const SOCKET_FILE_NAME: &str = "/home/jvle/Desktop/temp/sysDetector.sock";

#[derive(StructOpt, Debug)]
enum Command {
    START,
    STOP,
    LIST,
}

#[repr(u8)]
enum DeamonCommand {
    START = 1,
    STOP = 2,
    LIST = 3,
}


#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let cmd = Command::from_args();

    let daemon_cmd:u8 = match cmd {
        Command::START => DeamonCommand::START as u8,
        Command::STOP => DeamonCommand::STOP as u8,
        Command::LIST => DeamonCommand::LIST as u8,
    };

    let socket_path = SOCKET_FILE_NAME;
    let mut stream = UnixStream::connect(socket_path).await?;
    
    stream.write_all(&[daemon_cmd]).await?;

    let mut buf = String::new();
    let resp = stream.read_to_string(&mut buf).await?;
    
    Ok(())
}