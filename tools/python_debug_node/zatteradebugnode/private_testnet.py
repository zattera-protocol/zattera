"""
This test module will only run on a POSIX system. Windows support *may* be added at some point in the future.
"""
# Global imports
import json, operator, os, signal, sys

from argparse import ArgumentParser
from datetime import datetime
from pathlib import Path
from time import sleep
from time import time

# local imports
from zatteradebugnode import DebugNode

_running = True

def main():
   """
   This example contains a simple parser to obtain the locations of both zatterad and the data directory,
   creates and runs a new debug node, replays all of the blocks in the data directory, and finally waits
   for the user to interface with it outside of the script. Sending SIGINT succesfully and cleanly terminates
   the program.
   """
   import os, signal, sys
   from argparse import ArgumentParser
   global _running

   if os.name != "posix":
      print("This script only works on POSIX systems")
      return

   parser = ArgumentParser(description="Run a Debug Node on an existing chain. This simply replays all blocks " +
                              "and then waits indefinitely to allow user interaction through RPC calls and " +
                              "the CLI wallet")
   parser.add_argument("--zatterad-path", "-s", type=str, required=True, help="Path to the zatterad binary")
   parser.add_argument("--data-dir", "-d", type=str, required=True, help="Path to an existing data directory. " +
                        "The debug node will pull blocks from this directory when replaying the chain. The directory " +
                        "will not be changed.")
   parser.add_argument("--plugins", "-p", type=str, required=False, help="A list of plugins to load. witness and " +
                        "debug_node are always loaded.")
   parser.add_argument("--apis", "-a", type=str, required=False, help="A list of apis to load. database_api, login_api, " +
                        "and debug_node_api are always loaded")

   args = parser.parse_args()

   zatterad_path = Path(args.zatterad_path)
   if not zatterad_path.exists():
      print("Error: zatterad does not exist.")
      return

   zatterad_path = zatterad_path.resolve()
   if not zatterad_path.is_file():
      print("Error: zatterad is not a file.")
      return

   data_dir = Path(args.data_dir)
   if not data_dir.exists():
      print("Error: data_dir does not exist or is not a properly constructed zatterad data directory")

   data_dir = data_dir.resolve()
   if not data_dir.is_dir():
      print("Error: data_dir is not a directory")

   plugins = list()
   if args.plugins:
      plugins = args.plugins.split()

   apis = list()
   if args.apis:
      apis = args.apis.split()

   signal.signal(signal.SIGINT, sigint_handler)

   print("Creating and starting debug node")
   debug_node = DebugNode(str(zatterad_path), str(data_dir), plugins=plugins, apis=apis, zatterad_args="--replay", zatterad_stderr=sys.stderr)

   with debug_node:
      debug_node.debug_generate_blocks_until(int(time()), True)
      debug_node.debug_set_hardfork(14)
      print("Done!")
      print("Feel free to interact with this node via RPC calls for the cli wallet.")
      print("To shutdown the node, send SIGINT with Ctrl + C to this script. It will shut down safely.")

      while _running:
         assert debug_node.debug_generate_blocks(1) == 1

         sleep(3)

def sigint_handler(signum, frame):
   global _running

   _running = False
   sleep(3)
   sys.exit(0)

main()