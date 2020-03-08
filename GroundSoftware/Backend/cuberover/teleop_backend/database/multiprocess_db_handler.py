import collections
import multiprocessing
import queue
import time

from teleop_backend.database import mongo_db_collection
from teleop_backend.utils import signal_utils

USING_IDL = True
if USING_IDL:
    from teleop_backend.idl_msgs import msg_utilities
else:
    from teleop_backend.commands import command


class StatusUpdate:
    def __init__(self, lookup_id, *,
                 new_state: mongo_db_collection.MongoDbState = None,
                 err_string: str = None):
        self.lookup_id = lookup_id
        self.__new_state = new_state
        self.__err_string = err_string

    def get_update_dict(self):
        update_dict = {}
        if self.__new_state is not None:
            update_dict['stateFp'] = self.__new_state.value

        if self.__err_string is not None:
            update_dict['errorFp'] = self.__err_string

        return update_dict


class MultiprocessDbHandler:

    def __init__(self,
                 application_wide_shutdown_event: multiprocessing.Event,
                 new_command_output_queue: multiprocessing.Queue,
                 command_status_update_queue: multiprocessing.Queue,
                 join_timeout_until_terminate=2):
        self.__proc = None
        self.__application_wide_shutdown_event = application_wide_shutdown_event
        self.__this_handler_shutdown_event = multiprocessing.Event()
        self.__new_command_output_queue = new_command_output_queue
        self.__command_status_update_queue = command_status_update_queue
        self.__join_timeout_until_terminate = join_timeout_until_terminate

    def start(self):
        self.__proc = multiprocessing.Process(target=MultiprocessDbHandler.db_interaction_task,
                                              name="MultiprocessDbHandler_db_interaction_task",
                                              args=(self.__application_wide_shutdown_event,
                                                    self.__this_handler_shutdown_event,
                                                    "",
                                                    self.__new_command_output_queue,
                                                    self.__command_status_update_queue))
        self.__proc.start()

    def shutdown(self):
        self.__this_handler_shutdown_event.set()

        # We give one second after the select will time out for the recv thread to
        try:
            self.__proc.join(timeout=self.__join_timeout_until_terminate)
        except multiprocessing.TimeoutError:
            self.__proc.terminate()

    @staticmethod
    def new_command_from_db_output(new_cmd_dict):
        new_cmd = None
        if USING_IDL:
            cmd_cls = msg_utilities.NAME_TO_CLASS_MAP[new_cmd_dict["name"]]
            try:
                new_cmd = cmd_cls.from_database_object(new_cmd_dict)
            except TypeError as err:
                print("[MultiprocessDbHandler]: ERROR occurred when attempting to parse the database "
                        "object {} to the command class {}. Error: {}".format(new_cmd_dict,
                                                                            cmd_cls.__name__,
                                                                            err))
        else:
            new_cmd = command.Command.from_database(new_cmd_dict)
        return new_cmd

    @staticmethod
    def db_new_commands_handler(db_collection, 
                                recently_updated_lookup_ids,
                                new_command_output_queue):
        new_commands = db_collection.changes.get_all_new()
        for new_cmd_dict in new_commands:
            if new_cmd_dict["lookupID"] in recently_updated_lookup_ids:
                print("[MultiprocessDbHandler]: Ignoring new command in change stream because its lookup ID "
                        "was in the recently updated lookup ID buffer "
                        "(lookup ID = {})".format(new_cmd_dict["lookupID"]))
            else:
                print("[MultiprocessDbHandler]: Got new command:", new_cmd_dict)
                # new_command_from_db_output(new_cmd_dict)
                new_cmd = MultiprocessDbHandler.new_command_from_db_output(new_cmd_dict)

                if new_cmd:
                    new_command_output_queue.put(new_cmd)

    @staticmethod
    def db_interaction_task(app_wide_shutdown_event,
                            this_handler_shutdown_event,
                            db_connection_info,
                            new_command_output_queue: multiprocessing.Queue,
                            command_status_update_queue: multiprocessing.Queue):
        try:
            signal_utils.setup_signal_handler_for_process(shutdown_event=this_handler_shutdown_event)
            recently_updated_lookup_ids = collections.deque(maxlen=100)
            db_collection = mongo_db_collection.MongoDbCollection()
            while not app_wide_shutdown_event.is_set() and not this_handler_shutdown_event.is_set():
                # Handle any command updates if we have them
                try:
                    # We'll break out of this while loop when the queue is empty
                    while True:
                        status_update = command_status_update_queue.get_nowait()  # type: StatusUpdate
                        print("[MultiprocessDbHandler]: Got new status update")
                        db_collection.update(status_update.lookup_id,
                                             status_update.get_update_dict())
                        recently_updated_lookup_ids.append(status_update.lookup_id)
                except queue.Empty:
                    pass
                # Next, handle any new commands in the database
                MultiprocessDbHandler.db_new_commands_handler(db_collection,recently_updated_lookup_ids,new_command_output_queue)

                # Finally, sleep for 0.2 seconds so that this loop runs at 5 Hz.
                time.sleep(0.2)
        finally:
            app_wide_shutdown_event.set()
            print("[MultiprocessDbHandler]: db interaction task exiting")
