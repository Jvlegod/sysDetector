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
use tokio::{io::{AsyncReadExt, AsyncWriteExt}, net::UnixStream, fs};
use log::{debug, LevelFilter};
use serde_json;
use anyhow::Result;
mod args;

use self::{
    args::{Arguments},
};

// const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";
const SOCKET_FILE_NAME: &str = "/home/jvle/Desktop/temp/sysDetector.sock";
const LOG_FILE_PATH: &str = "/var/log/sysDetector/out.log";

async fn read_log_file() -> Result<String> {
    let mut file = fs::File::open(LOG_FILE_PATH).await?;
    let mut contents = String::new();
    file.read_to_string(&mut contents).await?;
    Ok(contents)
}
async fn handle_connection() -> Result<()> {
    let socket_path = SOCKET_FILE_NAME;
    let mut stream = UnixStream::connect(socket_path).await?;

    let args = Arguments::new()?;
    let serialized_cmd = serde_json::to_string(&args.subcommand)?;
    let cmd_bytes = serialized_cmd.as_bytes();

    let cmd_len = cmd_bytes.len() as u32;
    stream.write_all(&cmd_len.to_be_bytes()).await?;
    stream.write_all(cmd_bytes).await?;

    let mut buf = [0; 4];
    stream.read_exact(&mut buf).await?;
    let response_code =  i32::from_be_bytes(buf);

    debug!("Received response: {}", response_code);

    match args.subcommand {
        args::Command::PROC { identifier, opt } => {
            debug!("Received PROC command with identifier: {}, opt: {}", identifier, opt);
            match read_log_file().await {
                Ok(content) => {
                    println!("{}", content);
                }
                Err(e) => {
                    debug!("Failed to read log file: {}", e);
                }
            }
        }
        args::Command::FS { identifier, opt } => {
            debug!("Received FS command with identifier: {}, opt: {}", identifier, opt);
        }
        args::Command::LIST { identifier, opt } => {
            debug!("Received LIST command with identifier: {}, opt: {}", identifier, opt);
        }
    }

    Ok(())
}

#[tokio::main]
async fn main() -> Result<()> {
    let args = Arguments::new()?;
    let log_level = args.get_log_level()?;

    // env_logger::Builder::new()
    //    .filter_level(log_level)
    //    .init();

    debug!("Starting with arguments: {:?}", args.subcommand);

    handle_connection().await?;

    Ok(())
}