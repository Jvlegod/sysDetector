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
use log::{debug, LevelFilter};
use clap::Subcommand;
use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Subcommand, Serialize, Deserialize)]
pub enum Command {
    /// Set proc op
    PROC {
        identifier: String,

        /// Set proc opt
        #[clap(required = true)]
        opt: String,
    },

    /// Set fs op
    FS {
        identifier: String,

        /// Set fs opt
        #[clap(required = true)]
        opt: String,
    },

    /// Set list op
    LIST {
        identifier: String,

        /// Set list opt
        #[clap(required = true)]
        opt: String,
    },
}