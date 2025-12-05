"""
This test module will only run on a POSIX system. Windows support *may* be added at some point in the future.
"""
# Global imports
import json, operator, os, signal, sys

from argparse import ArgumentParser
from pathlib import Path
from time import sleep

# local imports
from zatteradebugnode import DebugNode
from zatteranoderpc import ZatteraNodeRPC

_running = True

def main():
   global _running
   if os.name != "posix":
      print("This script only works on POSIX systems")
      return

   parser = ArgumentParser(description="Run a zatterad debug node on an existing chain, trigger a hardfork" +
                              " and verify hardfork does not break invariants or block production")
   parser.add_argument("--zatterad-path", "-s", type=str, required=True, help="Path to the zatterad binary")
   parser.add_argument("--data-dir", "-d", type=str, required=True, help="Path to an existing data directory. " +
                        "The debug node will pull blocks from this directory when replaying the chain. The directory " +
                        "will not be changed.")
   parser.add_argument("--pause-node", "-p", type=bool, required=False, default=False,
                        help="True if the debug node should pause after it's tests. Default: false")

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

   signal.signal(signal.SIGINT, sigint_handler)

   debug_node = DebugNode(str(zatterad_path), str(data_dir))

   with debug_node:

      run_zatterad_tests(debug_node)

      if args.pause_node:
         print("Letting the node hang for manual inspection...")
      else:
         _running = False

      while _running:
         sleep(1)


def run_zatterad_tests(debug_node):

   try:
      print("Replaying blocks...",)
      sys.stdout.flush()
      total_blocks = 0
      while total_blocks % 100000 == 0:
         total_blocks += debug_node.debug_push_blocks(100000, skip_validate_invariants=True)
         print("Blocks Replayed: " + str(total_blocks))
         sys.stdout.flush()

      blocks_to_generate = 21 - total_blocks % 21
      debug_node.debug_generate_blocks(blocks_to_generate)

      print("Setting the hardfork now") # TODO: Grab most recent hardfork num from build directory
      sys.stdout.flush()
      debug_node.debug_set_hardfork(9)

      print("Generating blocks after the hardfork")
      assert debug_node.debug_generate_blocks(5000) == 5000

      print("Done!")

   except ValueError as val_err:
      print(str(val_err))

def sigint_handler(signum, frame):
   global _running
   _running = False
   sleep(3)
   sys.exit(0)

main()