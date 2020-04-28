#!/usr/bin/env python3

# Make sure we have the root directory "teleop_backend" in the python path
import inspect
import os
import sys

__THIS_FILE_DIR = os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))
__TELEOP_BACKEND_DIR = os.path.dirname(__THIS_FILE_DIR)
if __TELEOP_BACKEND_DIR not in sys.path:
    sys.path.insert(0, __TELEOP_BACKEND_DIR)

import copy
import enum
import multiprocessing
import random
import time

from teleop_backend.database import mongo_db_collection
from teleop_backend.utils import signal_utils

if sys.version_info >= (3, 6):
    COMMAND_TYPE_ENUM_BASE_CLASS = enum.IntFlag
else:
    COMMAND_TYPE_ENUM_BASE_CLASS = enum.IntEnum


@enum.unique
class NavigationCommandTypes(COMMAND_TYPE_ENUM_BASE_CLASS):
    FORWARD = 0
    BACKWARD = 1
    RIGHT = 2
    LEFT = 3
    STOP = 4


NAVIGATION_COMMAND_OPCODE = 0xCD


def main():
    """A fake "frontend" that injects messages into the database, like the frontend would do with user commands.

    This program doesn't need to be called with any arguments. Commands are sent at a random time interval between 0
    and 5 seconds.
    """
    db = mongo_db_collection.MongoDbCollection()

    # The contents of all of these commands are entirely arbitrary.
    left = {
        "type": "command",
        "opcode": NAVIGATION_COMMAND_OPCODE,
        "args": {"driving": 1, "command": NavigationCommandTypes.LEFT.value, "distance": 45, "speed": 9},
        "lookupID": 13
    }

    right = {
        "type": "command",
        "opcode": NAVIGATION_COMMAND_OPCODE,
        "args": {"driving": 1, "command": NavigationCommandTypes.RIGHT.value, "distance": 60, "speed": 5},
        "lookupID": 13
    }

    forward = {
        "type": "command",
        "opcode": NAVIGATION_COMMAND_OPCODE,
        "args": {"driving": 1, "command": NavigationCommandTypes.FORWARD.value, "distance": 10, "speed": 7},
        "lookupID": 13
    }

    backward = {
        "type": "command",
        "opcode": NAVIGATION_COMMAND_OPCODE,
        "args": {"driving": 1, "command": NavigationCommandTypes.BACKWARD.value, "distance": 20, "speed": 2},
        "lookupID": 13
    }

    stop = {
        "type": "command",
        "opcode": NAVIGATION_COMMAND_OPCODE,
        "args": {"driving": 1, "command": NavigationCommandTypes.STOP.value, "distance": 0, "speed": 0},
        "lookupID": 13
    }

    cmds = [left, right, forward, backward, stop]

    app_wide_shutdown_event = multiprocessing.Event()
    signal_utils.setup_signal_handler_for_process(shutdown_event=app_wide_shutdown_event)
    while not app_wide_shutdown_event.is_set():
        wait_time_until_next = 0.5 + (random.random() * 5.0)
        cmd_to_send = random.choice(cmds)
        cmd_to_send["lookupID"] = db.count() + 1
        db.write(copy.deepcopy(cmd_to_send))
        print("[{}]: Just wrote new cmd: {}".format(time.time(), cmd_to_send))
        time.sleep(wait_time_until_next)


if __name__ == "__main__":
    main()
