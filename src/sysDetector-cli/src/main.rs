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
#![allow(non_snake_case)]

use anyhow::Result;
use log::debug;
use serde_json;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::UnixStream,
};
mod args;

use self::args::Arguments;

const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";

async fn handle_connection(args: &Arguments) -> Result<()> {
    let socket_path = SOCKET_FILE_NAME;
    let mut stream = UnixStream::connect(socket_path).await?;

    let serialized_cmd = serde_json::to_string(&args.subcommand)?;
    let cmd_bytes = serialized_cmd.as_bytes();

    let cmd_len = cmd_bytes.len() as u32;
    stream.write_all(&cmd_len.to_be_bytes()).await?;
    stream.write_all(cmd_bytes).await?;

    let mut buf = [0; 4];
    stream.read_exact(&mut buf).await?;
    let response_code = i32::from_be_bytes(buf);

    let mut body_len_buf = [0; 4];
    stream.read_exact(&mut body_len_buf).await?;
    let body_len = u32::from_be_bytes(body_len_buf) as usize;
    let mut body = vec![0; body_len];
    if body_len > 0 {
        stream.read_exact(&mut body).await?;
        print!("{}", String::from_utf8_lossy(&body));
    }

    if response_code != 0 {
        return Err(anyhow::anyhow!("server returned error code {}", response_code));
    }

    debug!("Received response: {}", response_code);

    match &args.subcommand {
        args::Command::PROC { opt, identifier } => {
            debug!(
                "Received PROC command with opt: {}, identifier: {:?}",
                opt, identifier
            );
        }
        args::Command::FS { opt, identifier } => {
            debug!(
                "Received FS command with identifier: {:?}, opt: {}",
                identifier, opt
            );
        }
        args::Command::LIST { opt, identifier } => {
            debug!(
                "Received LIST command with identifier: {}, opt: {}",
                opt, identifier
            );
        }
    }

    Ok(())
}

#[tokio::main]
async fn main() -> Result<()> {
    let args = Arguments::new()?;
    let log_level = args.get_log_level()?;

    env_logger::Builder::new().filter_level(log_level).init();

    debug!("Starting with arguments: {:?}", args.subcommand);

    handle_connection(&args).await?;

    Ok(())
}
