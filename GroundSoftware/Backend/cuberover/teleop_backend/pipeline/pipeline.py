# system imports
import multiprocessing
import pathlib
import queue
import sys
import time
import typing

# fprime imports
from fprime_gds.common.loaders import ch_py_loader
from fprime_gds.common.loaders import event_py_loader
from fprime_gds.common.loaders import cmd_py_loader

from fprime_gds.common.decoders import decoder
from fprime_gds.common.decoders import ch_decoder
from fprime_gds.common.decoders import event_decoder
from fprime_gds.common.encoders import cmd_encoder

from fprime_gds.common.templates import cmd_template
from fprime_gds.common.templates import event_template
from fprime_gds.common.templates import ch_template

from fprime_gds.common.utils import data_desc_type

# teleop_backend imports
from teleop_backend.fprime_gds_additions.common.decoders import cmd_decoder
from teleop_backend.pipeline.consumers import event_consumer
from teleop_backend.pipeline.consumers import command_consumer
from teleop_backend.pipeline.consumers import telemetry_consumer
from teleop_backend.network import yamcs_client
from teleop_backend.network import message_parsing_state_machine
from teleop_backend.database import multiprocess_db_handler
from teleop_backend.database import mongo_db_collection
from teleop_backend.utils import signal_utils

_AllMessageDictionaries = typing.NamedTuple('_MessageDictionaries',
                                            [
                                                ('command_id_dict', typing.Dict[int, cmd_template.CmdTemplate]),
                                                ('command_name_dict', typing.Dict[str, cmd_template.CmdTemplate]),
                                                ('event_id_dict', typing.Dict[int, event_template.EventTemplate]),
                                                ('event_name_dict', typing.Dict[str, event_template.EventTemplate]),
                                                ('telemetry_id_dict', typing.Dict[int, ch_template.ChTemplate]),
                                                ('telemetry_name_dict', typing.Dict[str, ch_template.ChTemplate])
                                            ])

_CommandStateToDbState = {
    yamcs_client.CommandState.SENT_TO_YAMCS: mongo_db_collection.MongoDbState.SUCC_SENT_TO_YAMCS,
    yamcs_client.CommandState.QUEUED_BY_YAMCS: mongo_db_collection.MongoDbState.SUCC_QUEUED_BY_YAMCS,
    yamcs_client.CommandState.RELEASED_BY_YAMCS: mongo_db_collection.MongoDbState.SUCC_RELEASED_BY_YAMCS,
    yamcs_client.CommandState.SENT_BY_YAMCS: mongo_db_collection.MongoDbState.SUCC_SENT_BY_YAMCS,
    yamcs_client.CommandState.REJECTED_BY_YAMCS: mongo_db_collection.MongoDbState.FAIL,
}


class Pipeline:
    """Manages the overall data pipeline, in both directions, of the teleop backend application.

    This class is responsible for constructing all of the objects that are part of the pipeline as well as the queues
    that connect them, then performing all necessary steps to connect the steps of the pipeline together. Finally, this
    it also handles creating and then starting the processes that make the teleop backend function.

    Please see teleop_backend_data_flow.svg in the teleop_backend directory (or, if you have an editor that renders
    Markdown or can view it on GitHub, you can also look at the README.md file in the same location) to see how all of
    the components of the backend connect.

    Attributes:
        __received_db_object_queue: The queue that has database-format message dictionaries put onto it by the command,
                                    event, and telemetry consumers. These dictionaries are then pulled out of the queue
                                    by the database handler process.
        __received_serialized_msg_queue: The queue into which the message parsing state machine places messages parsed
                                         out of the stream of bytes from "Houston". This queue is consumed by the
                                         "message parser pump" process.
        __new_command_output_queue: The queue that has F Prime command data objects placed onto it by the database
                                    handler processes. These data objects represent commands placed into the
                                    database by the frontend. This queue is consumed by the "command from database
                                    sender" process.
        __command_status_update_queue: The queue into which StatusUpdate objects are placed when a commands status is
                                       updated by the "command from database sender process". Specifically, this process
                                       will determine if serializing and sending the command was successful, and will
                                       update the command's status based on how those actions went.
        __application_wide_shutdown_event: The multiprocessing.Event used to signal the shutdown of all processes that
                                           are part of the pipeline.
        __yamcs_client: The YAMCS client, through which we send commands to the YAMCS server. Additionally, this client
                        subscribes to updates from the YAMCS parameter through which the server makes all data from the
                        the rover available. The data from these updates is fed into the message parsing state machine.
        __db_handler_process: The database handler process.
        __command_from_database_sender_process: The "command from database sender" process, which consumes data objects
                                                from the __new_command_output_queue, serializes them, then sends them
                                                to "Houston". This process will also put a command status update into
                                                the __command_status_update_queue with the result of serializing and
                                                sending the command.
         __message_parser_pump_process: The "message parser pump" process, which consumes segmented serialized messages
                                        from the __received_serialized_msg_queue and feeds the F Prime decoders with
                                        message data.
    """

    def __init__(self):
        """Constructor."""
        # Our queues
        self.__received_db_object_queue = None
        self.__received_serialized_msg_queue = None
        self.__new_command_output_queue = None
        self.__command_status_update_queue = None

        # The event used to shut down all of the processes
        self.__application_wide_shutdown_event = None

        # Our subprocesses
        self.__yamcs_client = None
        self.__db_handler_process = None
        self.__command_from_database_sender_process = None
        self.__message_parser_pump_process = None

    def build_pipeline(self,
                       server_address: str,
                       server_port: int,
                       response_msg_name: str,
                       generated_file_directory_path: pathlib.Path,
                       yamcs_username: str,
                       yamcs_password: str,
                       yamcs_command_name: str,
                       yamcs_instance: str,
                       yamcs_processor_name: str,
                       yamcs_feedback_parameter_name: str):
        """Constructs all pipeline objects and queues and connects them, then starts all pipeline processes.

        For more details about the pipeline see the class description.

        Args:
            server_address: The IP address of the "Houston" server.
            server_port: The port number of the "Houston" server.
            response_msg_name: The name of the response message.
            generated_file_directory_path: The path of the root directory containing generated python dictionary files.
                                           This directory should contain directories called "channels", "commands", and
                                           "events".
            yamcs_username: The username of the credentials used to connect to the YAMCS server.
            yamcs_password: The password of the credentials used to connect to the YAMCS server.
            yamcs_command_name: The "name" of the YAMCS command that is sent for all of our commands. The true names of
                                our commands are prepended to the blob of binary data sent as the command argument.
            yamcs_instance: The name of the instance in the YAMCS server with which we interact.
            yamcs_processor_name: The name of the processor (in the YAMCS instance with name given by `yamcs_instance`)
                                  with which we interact.
            yamcs_feedback_parameter_name: The name of the paramter through which the YAMCS server makes available all
                                           data received from the rover.

        Returns:
            None

        Raises:
            ValueError: If the given response message name is not in the dictionary of command names.
        """
        # Get the dictionaries from opcodes to message templates and the ones from names to message templates for all
        # three types of message. This is done by loading the generated F Prime Python dictionary files.
        all_msg_dicts = self.__get_message_dictionaries(generated_file_directory_path)

        # Get the opcode corresponding to the given Response name
        if response_msg_name not in all_msg_dicts.command_name_dict:
            raise ValueError("Response message name ({}) is not in dictionary of command names (dictionary keys: "
                             "{})".format(response_msg_name, list(all_msg_dicts.command_name_dict.keys())))

        response_opcode = all_msg_dicts.command_name_dict[response_msg_name].get_op_code()

        # Create all of the multiprocessing.Queue instance variables
        self.__create_queues()

        # Create the dictionary of decoders needed by the Message Parser Pump process
        decoder_dict = self.__create_decoder_dictionary(all_msg_dicts.command_id_dict,
                                                        all_msg_dicts.event_id_dict,
                                                        all_msg_dicts.telemetry_id_dict)

        # Create then start all of our processes
        self.__create_all_processes(decoder_dict,
                                    all_msg_dicts.command_id_dict,
                                    response_opcode,
                                    yamcs_username,
                                    yamcs_password,
                                    yamcs_command_name)
        self.__start_all_processes(server_address,
                                   server_port,
                                   yamcs_instance,
                                   yamcs_processor_name,
                                   yamcs_feedback_parameter_name)

    def __get_message_dictionaries(self, generated_file_directory_path: pathlib.Path) -> _AllMessageDictionaries:
        """Loads all message types from the F Prime Python dictionary files and stores them in maps by opcode and name.

        Args:
            generated_file_directory_path: The path of the root directory containing generated python dictionary files.
                                           This directory should contain directories called "channels", "commands", and
                                           "events".

        Returns:
            An _AllMessageDictionaries namedtuple that contains all of the message dictionaries.
        """
        generated_events_path = generated_file_directory_path.joinpath("events")
        generated_telemetry_path = generated_file_directory_path.joinpath("channels")
        generated_commands_path = generated_file_directory_path.joinpath("commands")

        # Create Dictionaries
        event_loader = event_py_loader.EventPyLoader()
        event_id_dict, event_name_dict = event_loader.construct_dicts(str(generated_events_path))

        command_loader = cmd_py_loader.CmdPyLoader()
        command_id_dict, command_name_dict = command_loader.construct_dicts(str(generated_commands_path))

        telemetry_loader = ch_py_loader.ChPyLoader()
        telemetry_id_dict, telemetry_name_dict = \
            telemetry_loader.construct_dicts(str(generated_telemetry_path))

        return _AllMessageDictionaries(command_id_dict=command_id_dict,
                                       command_name_dict=command_name_dict,
                                       event_id_dict=event_id_dict,
                                       event_name_dict=event_name_dict,
                                       telemetry_id_dict=telemetry_id_dict,
                                       telemetry_name_dict=telemetry_name_dict)

    def __create_queues(self) -> None:
        """Create all of the queue instance variables that are part of the pipeline."""
        # This is the queue of message received from the rover, as tuples of
        # (message type (DataDescType), message header (bytes), and message contents (bytes))
        # The message header/contents is such that the contents are ready for input to the event, command, or telemetry
        # decoders.
        self.__received_serialized_msg_queue = multiprocessing.Queue()

        # This is the queue of messages received from the rover, as database-formatted objects. This is the input to the
        # multiprocess_db_handler, and is the output of message consumers. The message consumers are given the
        # decoded messages by the various decoders, and the decoders are given serialized message contents by the
        # message parsing state machine pump. The message parsing state machine pump is a multiprocessing process that
        # pulls messages from the output queue of the message parsing state machine. The message parsing state machine
        # is fed directly with data received from the network.
        self.__received_db_object_queue = multiprocessing.Queue()

        self.__new_command_output_queue = multiprocessing.Queue()
        self.__command_status_update_queue = multiprocessing.Queue()

    def __create_decoder_dictionary(self,
                                    command_id_dict: typing.Dict[int, cmd_template.CmdTemplate],
                                    event_id_dict: typing.Dict[int, event_template.EventTemplate],
                                    telemetry_id_dict: typing.Dict[int, ch_template.ChTemplate]) \
            -> typing.Dict[data_desc_type.DataDescType, decoder.Decoder]:
        """Creates a dictionary of decoders to use for decoding different message types.

        This dictionary is used by the message parsing state machine pump to select the correct decoder to which
        serialized message contents should be passed.

        Args:
            command_id_dict: A map from command opcodes to the templates of those commands.
            event_id_dict: A map from event opcodes to the templates of those events.
            telemetry_id_dict: A map from telemetry opcodes to the templates of those telemetry messages.

        Returns:
             A map from the data description type (DataDescType) of the different kinds of messages to the decoder
             object that is appropriate for decoding that kind of message.
        """
        the_event_consumer = event_consumer.EventConsumer(queue=self.__received_db_object_queue)
        the_command_consumer = command_consumer.CommandConsumer(queue=self.__received_db_object_queue)
        the_telemetry_consumer = telemetry_consumer.TelemetryConsumer(queue=self.__received_db_object_queue)

        the_event_decoder = event_decoder.EventDecoder(event_dict=event_id_dict)
        the_command_decoder = cmd_decoder.CmdDecoder(cmd_dict=command_id_dict)
        the_telemetry_decoder = ch_decoder.ChDecoder(ch_dict=telemetry_id_dict)

        # We need to register the consumers to each of their respective decoders, which will make the decoders pass the
        # decoded message data object to the consumers. The consumers will then convert it to a database-formatted
        # object and then put that db-format object into the input queue to the multiprocess_db_handler
        # (received_db_object_queue)
        the_event_decoder.register(consumer_obj=the_event_consumer)
        the_command_decoder.register(consumer_obj=the_command_consumer)
        the_telemetry_decoder.register(consumer_obj=the_telemetry_consumer)

        decoder_dict = {
            # This isn't an error; the DataDescType for events is called "LOG"
            data_desc_type.DataDescType["FW_PACKET_LOG"]: the_event_decoder,
            data_desc_type.DataDescType["FW_PACKET_COMMAND"]: the_command_decoder,
            data_desc_type.DataDescType["FW_PACKET_TELEM"]: the_telemetry_decoder
        }

        return decoder_dict

    def __create_all_processes(self,
                               decoder_dict: typing.Dict[data_desc_type.DataDescType, decoder.Decoder],
                               command_id_dict: typing.Dict[int, cmd_template.CmdTemplate],
                               response_opcode: int,
                               yamcs_username: str,
                               yamcs_password: str,
                               yamcs_command_name: str) -> None:
        """Instantiates all of the multiprocessing process object instance variables, but does not start them.

        Args:
            decoder_dict: A map from the data description type (DataDescType) of the different kinds of messages to
                          the decoder object that is appropriate for decoding that kind of message.
            command_id_dict: A map from command opcodes to the corresponding command templates.
            response_opcode: The opcode of the response command.
            yamcs_username: The username of the credentials used to connect to the YAMCS server.
            yamcs_password: The password of the credentials used to connect to the YAMCS server.
            yamcs_command_name: The "name" of the YAMCS command that is sent for all of our commands. The true names of
                                our commands are prepended to the blob of binary data sent as the command argument.

        Returns:
            None
        """
        # Create the shutdown event for all of the processes we're about to create
        self.__application_wide_shutdown_event = multiprocessing.Event()

        msg_parser = message_parsing_state_machine.MessageParsingStateMachine()
        msg_parser.register_completed_message_queue(completed_message_queue=self.__received_serialized_msg_queue)
        self.__yamcs_client = yamcs_client.YamcsClient(
            yamcs_username=yamcs_username,
            yamcs_password=yamcs_password,
            yamcs_command_name=yamcs_command_name,
            application_wide_shutdown_event=self.__application_wide_shutdown_event)
        self.__yamcs_client.register_data_handler(handler=msg_parser)

        self.__message_parser_pump_process = \
            MultiprocessingRunner(task_name="message_parsing_state_machine_pump",
                                  application_wide_shutdown_event=self.__application_wide_shutdown_event,
                                  function_to_repeatedly_run=Pipeline.transfer_msg_from_parser_queue_to_decoder,
                                  function_args=[self.__received_serialized_msg_queue, decoder_dict])

        self.__db_handler_process = multiprocess_db_handler.DbHandlerProcess(command_id_dict,
                                                                             response_opcode,
                                                                             self.__application_wide_shutdown_event,
                                                                             self.__new_command_output_queue,
                                                                             self.__received_db_object_queue,
                                                                             self.__command_status_update_queue)

        the_cmd_encoder = cmd_encoder.CmdEncoder()
        self.__command_from_database_sender_process = \
            MultiprocessingRunner(task_name="command_from_database_sender",
                                  application_wide_shutdown_event=self.__application_wide_shutdown_event,
                                  function_to_repeatedly_run=Pipeline.send_cmd_from_db,
                                  function_args=[self.__new_command_output_queue,
                                                 self.__command_status_update_queue,
                                                 the_cmd_encoder,
                                                 self.__yamcs_client])

    def __start_all_processes(self,
                              server_address: str,
                              server_port: int,
                              yamcs_instance: str,
                              yamcs_processor_name: str,
                              yamcs_feedback_parameter_name: str) -> None:
        """Starts all of the instance processes, retrying connection to the YAMCS server until it is successful.

        Args:
            server_address: The IP address of the "Houston" server.
            server_port: The port number of the "Houston" server.
            yamcs_instance: The name of the instance in the YAMCS server with which we interact.
            yamcs_processor_name: The name of the processor (in the YAMCS instance with name given by `yamcs_instance`)
                                  with which we interact.
            yamcs_feedback_parameter_name: The name of the paramter through which the YAMCS server makes available all
                                           data received from the rover.

        Returns:
            None
        """
        try:
            while not self.__application_wide_shutdown_event.is_set():
                try:
                    self.__yamcs_client.connect(address=server_address,
                                                port=server_port,
                                                yamcs_instance=yamcs_instance,
                                                yamcs_processor=yamcs_processor_name,
                                                yamcs_feedback_parameter_name=yamcs_feedback_parameter_name)
                    break
                except Exception as e:
                    print("Failed to connect to server due to {}, "
                          "will wait 5 seconds then retry connection".format(e), file=sys.stderr)
                    time.sleep(5)
        except KeyboardInterrupt:
            print("Building of pipeline exiting because of Ctrl-C while trying to connect to YAMCS server")
            self.__application_wide_shutdown_event.set()
            return

        self.__db_handler_process.start()
        self.__command_from_database_sender_process.start()
        self.__message_parser_pump_process.start()

    def spin(self):
        """Spin until the pipeline terminates.

        It is expected that the launching process (i.e. main()) will call this after starting the rest of the pipeline.

        Returns:
            None
        """
        signal_utils.setup_signal_handler_for_process(shutdown_event=self.__application_wide_shutdown_event)

        # This will block until we exit.
        while not self.__application_wide_shutdown_event.is_set():
            time.sleep(0.25)

    @staticmethod
    def send_cmd_from_db(new_command_output_queue: multiprocessing.Queue,
                         command_status_update_queue: multiprocessing.Queue,
                         the_cmd_encoder: cmd_encoder.CmdEncoder,
                         the_yamcs_client: yamcs_client.YamcsClient):
        """The function of a process that retrieves converted commands from the database and sends them.

        More specifically, this is a function set up to be used with MultiprocessingRunner. Each invocation of this
        function attempts to retrieve one command from the new command output queue (which contains tuples of the
        lookup ID and the F Prime command data object for a command retrieved from the database), serialize it,
        prepend the header, destination string, and command name, then send it to the Astrobotics AMCC YAMCS server via
        the YAMCS client.

        In addition to the above, this task will send a StatusUpdate back to the database via the command status update
        queue. This StatusUpdate will be sent even if serializing or sending the command fails (though in these cases
        the contents of the update will obviously be different from that when these things succeed). This function will
        also frequently call the get_command_updates() function of the YamcsClient in order to get any updates about the
        status of commands within the YAMCS server, and then will create StatusUpdates to update the database
        accordingly.
        """
        try:
            lookup_id, cmd_from_db_data_object = new_command_output_queue.get(block=True, timeout=0.001)
            template = cmd_from_db_data_object.get_template()
            opcode = template.get_op_code()
            cmd_name = template.get_name()
            cmd_name_bytes = cmd_name.encode(encoding="utf-8")

            update = None
            try:
                serialized_cmd_bytes = the_cmd_encoder.encode_api(cmd_from_db_data_object)
                full_serialized_cmd_bytes = b"%sA5A5 FSW %s" % (cmd_name_bytes, serialized_cmd_bytes)
                print("Sending bytes: \"{}\"".format(full_serialized_cmd_bytes))
                cmd_update = the_yamcs_client.send(full_serialized_cmd_bytes, lookup_id=lookup_id, opcode=opcode)

                update = \
                    multiprocess_db_handler.StatusUpdate(lookup_id=lookup_id,
                                                         cmd_opcode=opcode,
                                                         new_state=mongo_db_collection.MongoDbState.SUCC_SENT_TO_YAMCS)
            except Exception as err:
                print("[send_cmd_from_db]: Sending {} failed due to error:"
                      " {}".format(cmd_name, err))
                update = multiprocess_db_handler.StatusUpdate(
                    lookup_id=lookup_id,
                    cmd_opcode=opcode,
                    new_state=mongo_db_collection.MongoDbState.FAIL,
                    err_string="Failed to send due to error: {}".format(err))
                raise
            finally:
                if update is not None:
                    print("[send_cmd_from_db]: Writing status update for lookup id {}:"
                          " {}".format(update.lookup_id, update.get_update_dict()))
                    command_status_update_queue.put(update)
        except queue.Empty:
            pass

        # After checking for new commands, handle any changes to command statuses
        command_updates = the_yamcs_client.get_command_updates()

        for update in command_updates:
            err_str = None
            if update.new_state == yamcs_client.CommandState.REJECTED_BY_YAMCS:
                err_str = "Failed to send due to error: {}".format(update.err_message_if_rejected)

            status_update = multiprocess_db_handler.StatusUpdate(
                lookup_id=update.lookup_id,
                cmd_opcode=update.opcode,
                new_state=_CommandStateToDbState[update.new_state],
                err_string=err_str)

            print("[send_cmd_from_db]: Writing status update for lookup id {}:"
                  " {} (transition from {} to {})".format(status_update.lookup_id,
                                                          status_update.get_update_dict(),
                                                          update.old_state.name,
                                                          update.new_state.name))
            command_status_update_queue.put(status_update)

    @staticmethod
    def transfer_msg_from_parser_queue_to_decoder(received_serialized_message_queue: multiprocessing.Queue,
                                                  decoder_dict: typing.Dict[
                                                      data_desc_type.DataDescType, decoder.Decoder]):
        """The function of a process that retrieves commands from the message parser and forwards them to the decoders.

        More specifically, this is a function set up to be used with MultiprocessingRunner. Each invocation of this
        function attempts to retrieve one command from the output queue of the message parsing state machine, and then
        pass the serialized contents of that message to the appropriate decoder (where "appropriate" is determined by
        the type of the message retrieved from the queue).

        Args:
            received_serialized_message_queue: The output queue of the message parsing state machine. The values in this
                                               queue will be tuples of message type (as a data_desc_type.DataDescType),
                                               message header bytes, and message contents bytes.
            decoder_dict: A dictionary mapping from message types to the F Prime Decoder for that type of message.

        Returns:
            None
        """
        # For now, we don't do anything with the header bytes. The msg contents go to the appropriate decoder
        try:
            msg_type, _, new_msg_content_bytes = received_serialized_message_queue.get(block=True, timeout=0.5)
            decoder_dict[msg_type].data_callback(new_msg_content_bytes)
        except queue.Empty:
            pass


class MultiprocessingRunner:
    """A wrapper class that manages a generic multiprocessing.Process that can repeatedly call a particular function.

    Specifically, the task function executed by the managed Process will repeatedly call a given function with given
    arguments and keyword arguments until the application wide shutdown event is signalled.

    Attributes:
        __proc: The multiprocessing.Process managed by this object.
    """

    def __init__(self, *,
                 task_name: str,
                 application_wide_shutdown_event: multiprocessing.Event,
                 function_to_repeatedly_run: typing.Callable,
                 function_args: list = None,
                 function_kwargs: dict = None):
        """Constructor.

        Args:
            task_name: A name for the task of this runner.
            application_wide_shutdown_event: The event that is used to signal the shutdown of all Processes in this
                                             application.
            function_to_repeatedly_run: A callable that will be called repeatedly by the task of the managed Process.
            function_args: The arguments that will be passed to function_to_repeatedly_run on each invocation.
            function_kwargs: The keyword arguments that will be passed to function_to_repeatedly_run on each invocation.
        """
        function_args = function_args if function_args is not None else []
        function_kwargs = function_kwargs if function_kwargs is not None else {}
        self.__proc = multiprocessing.Process(target=MultiprocessingRunner.runner_task,
                                              name="{}_runner".format(task_name),
                                              args=(task_name,
                                                    application_wide_shutdown_event,
                                                    function_to_repeatedly_run,
                                                    function_args,
                                                    function_kwargs))

    def start(self):
        """Starts the managed Process.

        Returns:
            None
        """
        self.__proc.start()

    @staticmethod
    def runner_task(task_name: str,
                    application_wide_shutdown_event: multiprocessing.Event,
                    function_to_repeatedly_run: typing.Callable,
                    function_args: list,
                    function_kwargs: dict):
        """The function performed by the Process managed by this runner.

        This function will repeatedly call the given function until the application wide shutdown event is signalled.

        Args:
            Same as constructor arguments with the same names, see constructor documentation for more details.
        """
        try:
            signal_utils.setup_signal_handler_for_process(shutdown_event=application_wide_shutdown_event)

            while not application_wide_shutdown_event.is_set():
                function_to_repeatedly_run(*function_args, **function_kwargs)

        finally:
            application_wide_shutdown_event.set()
            print("[{}]: Task exiting".format(task_name))
