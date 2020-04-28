#!/usr/bin/env python3

# Make sure we have the root directory "teleop_backend" in the python path
import inspect
import os
import sys

__THIS_FILE_DIR = os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))
__TELEOP_BACKEND_DIR = os.path.dirname(__THIS_FILE_DIR)
if __TELEOP_BACKEND_DIR not in sys.path:
    sys.path.insert(0, __TELEOP_BACKEND_DIR)

# system imports
import argparse
import pathlib

from teleop_backend.utils import fprime_import_utils

# This is a workaround for differing behavior of the pathlib.Path.resolve() method between Python <3.6 and >=3.6.
# Calling path.resolve(**__PATH_RESOLVE_KWARGS) gives the behavior of path.resolve() in <3.6 which is equivalent to
# path.resolve(strict=True) in >=3.6
if sys.version_info >= (3, 6):
    __PATH_RESOLVE_KWARGS = {"strict": True}
else:
    __PATH_RESOLVE_KWARGS = {}


def main():
    """A fake "rover" that acts as a UDP server for the backend to connect to and responds to commands.

    Specifically, this fake "rover" will send a Response message indicating success for every command it receives from
    the backend. The data flow within this fake rover work similar to the backend itself but a bit simpler and in
    reverse, as we receive an F Prime encoded message, convert it to database-format for easy printing and parsing,
    then create a Response message, serialize it, and send it back to the backend.
    """
    parser = argparse.ArgumentParser(description="Acts as a fake receiver and prints the commands it receives")
    parser.add_argument("-a", "--server-address", type=str, required=True,
                        help="The address to which the server socket will bind messages")
    parser.add_argument("-p", "--server-port", type=int, required=True,
                        help="The port to which the server socket will bind and receive messages")
    parser.add_argument("-r", "--response-port", type=int, required=True,
                        help="The port to which the server should send responses (client address will be used)")
    parser.add_argument("-g", "--generated_file_directory", type=str, required=True,
                        help="The directory that contains the generated Python source files for the commands, events, "
                             "and telemetry channels defined within the F Prime distribution. It is expected that this "
                             "directory will contain three subdirectories: 'commands', 'channels', and 'events', which "
                             "contain the command files, telemetry channel files, and event files, respectively.")
    parser.add_argument("--fprime_gds_python_directory", type=str, required=False, default=None,
                        help="The root directory of the F Prime GDS Python package. This directory normally lives at "
                             "'<fprime_root>/Gds/src/fprime_gds'. If this argument is not specified, it is attempted "
                             "to automatically detect this by finding the root of the git repository that contains this"
                             " file. If this file is not in the CubeRoverPackage repository, then this is argument must"
                             " be given for this application to function.")
    parser.add_argument("--fprime_python_directory", type=str, required=False, default=None,
                        help="The root directory of the F Prime Fw Python package. This directory normally lives at "
                             "'<fprime_root>/Fw/Python/src/fprime'. If this argument is not specified, it is attempted "
                             "to automatically detect this by finding the root of the git repository that contains this"
                             " file. If this file is not in the CubeRoverPackage repository, then this is argument must"
                             " be given for this application to function.")
    parser.add_argument("--response_command_name", type=str, required=False, default="Response",
                        help="The exact name of the command sent by the rover as a response to all of the commands"
                             " that it receives. If not specified, \"Response\" is used by default.")
    args = parser.parse_args()

    # Get the fprime and fprime_gds package directories, either from command line arguments or based on the expected
    # location of the F Prime distribution root directory in the CubeRoverPackage git repo that we expect contains the
    # entire teleop_backend package.
    fprime_root_path = None

    try:
        if args.fprime_gds_python_directory is None:
            fprime_root_path = fprime_import_utils.get_fprime_root_path_from_git_root()
            fprime_gds_path = fprime_import_utils.get_fprime_gds_python_package_path_from_fprime_root(fprime_root_path)
        else:
            fprime_gds_path = pathlib.Path(args.fprime_gds_python_directory).resolve(**__PATH_RESOLVE_KWARGS)

            if not fprime_gds_path.is_dir():
                raise FileNotFoundError("A file exists at the path ({}) expected to contain the fprime_gds Python "
                                        "package".format(str(fprime_gds_path)))
    except Exception as e:
        print("Failed to find directory of the fprime_gds Python package due to error: {}: {}. Cannot "
              "continue, as the fprime_gds package needs to be imported for the backend to "
              "operate.".format(type(e).__name__, e), file=sys.stderr)
        return -1

    try:
        if args.fprime_python_directory is None:
            # We don't need to get the F Prime root again if we've already gotten it
            if fprime_root_path is None:
                fprime_root_path = fprime_import_utils.get_fprime_root_path_from_git_root()

            fprime_python_path = fprime_import_utils.get_fprime_fw_python_package_path_from_fprime_root(
                fprime_root_path)
        else:
            fprime_python_path = pathlib.Path(args.fprime_python_directory).resolve(**__PATH_RESOLVE_KWARGS)

            if not fprime_python_path.is_dir():
                raise FileNotFoundError("A file exists at the path ({}) expected to contain the fprime Python package "
                                        "(from the Fw module)".format(str(fprime_python_path)))

    except Exception as e:
        print("Failed to find directory of the fprime Python package (from the Fw module) due to error: {}: {}. Cannot "
              "continue, as the fprime package needs to be imported for the backend to "
              "operate.".format(type(e).__name__, e), file=sys.stderr)
        return -1

    # Add fprime and fprime_gds package directories to the path
    fprime_python_path_str = str(fprime_python_path)
    fprime_gds_path_str = str(fprime_gds_path)

    if fprime_python_path_str not in sys.path:
        sys.path.insert(0, fprime_python_path_str)

    if fprime_gds_path_str not in sys.path:
        sys.path.insert(0, fprime_gds_path_str)

    from teleop_backend import teleop_fake_rover_pipeline
    rover_pipeline = teleop_fake_rover_pipeline.FakeRoverPipeline()
    rover_pipeline.build_pipeline(server_address=args.server_address,
                                  server_port=args.server_port,
                                  response_port=args.response_port,
                                  response_msg_name=args.response_command_name,
                                  generated_file_directory_path=pathlib.Path(args.generated_file_directory))

    rover_pipeline.spin()


if __name__ == "__main__":
    main()
