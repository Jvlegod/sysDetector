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
use core::str;
use std::path::Path;
use std::sync::{Arc, Mutex};
use log::info;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::UnixListener,
};
use env_logger::Builder;
use std::env;

mod rpc;
mod args;

use anyhow::Result;

const SOCKET_FILE_NAME: &str = "/var/run/sysDetector.sock";

async fn handle_connection(mut stream: tokio::net::UnixStream, rpc: Arc<Mutex<rpc::Rpc>>) -> Result<()> {
    let mut len_buf = [0; 4];
    stream.read_exact(&mut len_buf).await?;
    let message_len = u32::from_be_bytes(len_buf) as usize;

    let mut buf = vec![0; message_len];
    stream.read_exact(&mut buf).await?;

    let response = {
        let mut rpc_lock = rpc.lock().unwrap();
        rpc_lock.handle_command(&buf)?
    };

    stream.write_all(&response.to_be_bytes()).await?;

    Ok(())
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    if Path::new(SOCKET_FILE_NAME).exists() {
        std::fs::remove_file(SOCKET_FILE_NAME)?;
    }

    let listener = UnixListener::bind(SOCKET_FILE_NAME)?;
    println!("sysDetector server listening on {}", SOCKET_FILE_NAME);
    let rpc = Arc::new(Mutex::new(rpc::Rpc::new()?));

    loop {
        let (stream, _) = listener.accept().await?;
        let rpc_clone = Arc::clone(&rpc);
        tokio::spawn(handle_connection(stream, rpc_clone));
    }
}    