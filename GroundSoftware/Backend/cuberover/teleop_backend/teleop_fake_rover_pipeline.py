#!/usr/bin/env python3

# system imports
import multiprocessing
import pathlib
import queue
import select
import socket
import struct
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

from fprime_gds.common.utils import data_desc_type

# teleop_backend imports
from teleop_backend.fprime_gds_additions.common.decoders import cmd_decoder
from teleop_backend.msg_format import format_conversion
from teleop_backend.pipeline import pipeline
from teleop_backend.pipeline.consumers import event_consumer
from teleop_backend.pipeline.consumers import command_consumer
from teleop_backend.pipeline.consumers import telemetry_consumer
from teleop_backend.network import message_parsing_state_machine
from teleop_backend.utils import signal_utils

SERVER_SELECT_TIMEOUT = 0.01
RECV_MAX_SIZE = 4096


class FakeRoverPipeline:
    """A pipeline very similar to the one used by the backend, tweaked to act like a combined "Houston" and "rover".

    The data flow within this fake rover work similar to the backend itself but a bit simpler and in
    reverse, as we receive an F Prime encoded message, convert it to database-format for easy printing and parsing,
    then create a Response message, serialize it, and send it back to the backend.

    Specifically, this pipeline doesn't need the database handler or the process to send messages from the data handler.
    Instead, it needs a UDP server (which is implemented within this class) that feeds the message parsing state
    machine. As with the backend pipeline, the data goes from the message parsing state machine to decoders and then to
    consumers, the output of which is database-formatted objects. In the backend pipeline the launching process ends up
    spinning doing nothing and waiting for the other processes to end. However, in this pipeline the launching process
    takes the output of the consumers, generates a Response message in database format indicating the success of the
    received command, then converts that Response message to a command object and then serialized data, and finally
    puts that data onto a queue to be sent as part of the UDP server process. That completes the loop back from received
    command to sent response.

    Attributes:
        __server_proc: The UDP server process.
        All others are identical to attributes of pipeline.Pipeline, look at the documentation there for more details.
    """

    def __init__(self):
        """Constructor."""
        # Our queues
        self.__received_db_object_queue = None
        self.__received_serialized_msg_queue = None
        self.__send_queue = None

        # Some f prime stuff
        self.__the_cmd_encoder = None
        self.__response_opcode = None
        self.__command_id_dict = None

        # The event used to shut down all of the processes
        self.__application_wide_shutdown_event = None

        # Our subprocesses
        self.__message_parser_pump_process = None
        self.__server_proc = None

    def build_pipeline(self,
                       server_address: str,
                       server_port: int,
                       response_port: int,
                       response_msg_name: str,
                       generated_file_directory_path: pathlib.Path):
        """Constructs all pipeline objects and queues and connects them, then starts all pipeline processes.

        For more details about the pipeline see the class description.

        Args:
            server_address: The IP address to which the UDP server socket of this "rover" pipeline will bind.
            server_port: The port number to which the UDP server socket of this "rover" pipeline will bind.
            response_port: The port number to which the UDP server socket of this "rover" pipeline will send data. The
                           destination address will consist of the host from the client that send the command and this
                           port.
            response_msg_name: The name of the response message.
            generated_file_directory_path: The path of the root directory containing generated python dictionary files.
                                           This directory should contain directories called "channels", "commands", and
                                           "events".

        Returns:
            None

        Raises:
            ValueError: If the given response message name is not in the dictionary of command names.
        """
        generated_events_path = generated_file_directory_path.joinpath("events")
        generated_telemetry_path = generated_file_directory_path.joinpath("channels")
        generated_commands_path = generated_file_directory_path.joinpath("commands")

        # Create Dictionaries
        event_loader = event_py_loader.EventPyLoader()
        event_id_dict, event_name_dict = event_loader.construct_dicts(str(generated_events_path))

        command_loader = cmd_py_loader.CmdPyLoader()
        self.__command_id_dict, command_name_dict = command_loader.construct_dicts(str(generated_commands_path))

        if response_msg_name not in command_name_dict:
            raise ValueError("Response message name ({}) is not in dictionary of command names (dictionary keys: "
                             "{})".format(response_msg_name, list(command_name_dict.keys())))

        self.__response_opcode = command_name_dict[response_msg_name].get_op_code()

        telemetry_loader = ch_py_loader.ChPyLoader()
        telemetry_id_dict, telemetry_name_dict = \
            telemetry_loader.construct_dicts(str(generated_telemetry_path))

        # This is the queue of message received from the rover, as tuples of
        # (message type (DataDescType), message header (bytes), and message contents (bytes))
        # The message header/contents is such that the contents are ready for input to the event, command, or telemetry
        # decoders.
        self.__received_serialized_msg_queue = multiprocessing.Queue()
        msg_parser = message_parsing_state_machine.MessageParsingStateMachine()
        msg_parser.register_completed_message_queue(completed_message_queue=self.__received_serialized_msg_queue)

        # This is the queue of messages received from the rover, as database-formatted objects. This is the input to the
        # multiprocess_db_handler, and is the output of message consumers. The message consumers are given the
        # decoded messages by the various decoders, and the decoders are given serialized message contents by the
        # message parsing state machine pump. The message parsing state machine pump is a multiprocessing process that
        # pulls messages from the output queue of the message parsing state machine. The message parsing state machine
        # is fed directly with data received from the network.
        self.__received_db_object_queue = multiprocessing.Queue()

        the_event_consumer = event_consumer.EventConsumer(queue=self.__received_db_object_queue)
        the_command_consumer = command_consumer.CommandConsumer(queue=self.__received_db_object_queue)
        the_telemetry_consumer = telemetry_consumer.TelemetryConsumer(queue=self.__received_db_object_queue)

        the_event_decoder = event_decoder.EventDecoder(event_dict=event_id_dict)
        the_command_decoder = cmd_decoder.CmdDecoder(cmd_dict=self.__command_id_dict)
        the_telemetry_decoder = ch_decoder.ChDecoder(ch_dict=telemetry_id_dict)

        # We need to register the consumers to each of their respective decoders, which will make the decoders pass the
        # decoded message data object to the consumers. The consumers will then convert it to a database-formatted
        # object and then put that db-format object into the input queue to the multiprocess_db_handler
        # (received_db_object_queue)
        the_event_decoder.register(consumer_obj=the_event_consumer)
        the_command_decoder.register(consumer_obj=the_command_consumer)
        the_telemetry_decoder.register(consumer_obj=the_telemetry_consumer)

        # This is a map from the data description type (DataDescType) of the different kinds of messages to the
        # decoder object that is appropriate for decoding that kind of message. This dictionary is used by the message
        # parsing state machine pump to select the correct decoder to which serialized message contents should be
        # passed.
        decoder_dict = {
            # This isn't an error; annoyingly the DataDescType for events is called "LOG"
            data_desc_type.DataDescType["FW_PACKET_LOG"]: the_event_decoder,
            data_desc_type.DataDescType["FW_PACKET_COMMAND"]: the_command_decoder,
            data_desc_type.DataDescType["FW_PACKET_TELEM"]: the_telemetry_decoder
        }

        self.__application_wide_shutdown_event = multiprocessing.Event()

        self.__message_parser_pump_process = \
            pipeline.MultiprocessingRunner(
                task_name="message_parsing_state_machine_pump",
                application_wide_shutdown_event=self.__application_wide_shutdown_event,
                function_to_repeatedly_run=FakeRoverPipeline.transfer_msg_from_parser_queue_to_decoder,
                function_args=[self.__received_serialized_msg_queue, decoder_dict])

        self.__send_queue = multiprocessing.Queue()
        self.__the_cmd_encoder = cmd_encoder.CmdEncoder()

        self.__server_proc = multiprocessing.Process(target=FakeRoverPipeline.server_recv_task,
                                                     name="teleop_fake_server_recv_task",
                                                     args=(self.__application_wide_shutdown_event,
                                                           server_address,
                                                           server_port,
                                                           response_port,
                                                           SERVER_SELECT_TIMEOUT,
                                                           msg_parser,
                                                           self.__send_queue))

        self.__message_parser_pump_process.start()
        self.__server_proc.start()

    def spin(self) -> None:
        """Like the process function of pipeline.MultiprocessingRunner, but can be called from the launching process.

        Unlike the process function of pipeline.MultiprocessingRunner, this is hardcoded to invoke a single function,
        which is command_receiver_and_responder().

        Returns:
            None
        """
        # This will block until we exit.
        try:
            signal_utils.setup_signal_handler_for_process(shutdown_event=self.__application_wide_shutdown_event)

            while not self.__application_wide_shutdown_event.is_set():
                self.command_receiver_and_responder()

        finally:
            self.__application_wide_shutdown_event.set()
            print("[{}]: Task exiting".format("command_receiver_and_responder"))

    def command_receiver_and_responder(self) -> None:
        """Grabs a single database-format command that has been received and creates then sends a Response for it.

        Specifically, this function is called repeatedly by the launching process via spin(). Each invocation of this
        function grabs a single database-format command and from that generates an appropriate database-format Response
        command to act as the reply. This response is then sent via send_response() (see the documentation of that
        function for more details of the steps to sending).

        Returns:
            None
        """
        try:
            db_format_msg = self.__received_db_object_queue.get(block=True, timeout=0.5)

            if db_format_msg["type"] != "command":
                print("Got a message that is not a command: {}".format(db_format_msg), file=sys.stderr)
                return

            # Print our received message
            print("[{}]: {}".format(time.time(), str(db_format_msg)))

            # Send the response, for now always success response
            response = {
                "type": "command",
                "opcode": self.__response_opcode,
                "args": {
                    "type": db_format_msg["opcode"],
                    "command": db_format_msg["args"]["command"],
                    "errorCode": 0,
                    "additionalData": 0
                }
            }

            self.send_response(response)
        except queue.Empty:
            pass

    def send_response(self, response_in_db_format: dict) -> None:
        """Converts and serializes a database-format Response message, then sends the serialized bytes.

        The given Response command is converted into an F Prime command data object, which is in turn converted into
        serialized data. This serialized data is sent via send_message().

        Args:
            response_in_db_format: The database-format Response command to be sent.

        Returns:
            None
        """
        response_data_obj = \
            format_conversion.convert_command_from_database_to_data_type(response_in_db_format, self.__command_id_dict)
        serialized_cmd_bytes = self.__the_cmd_encoder.encode_api(response_data_obj)
        self.send_message(serialized_cmd_bytes)

    def send_message(self, serialized_cmd_bytes: bytes) -> None:
        """Appends the header and destination bytes to a serialized message then enqueues it to be sent.

        Args:
            serialized_cmd_bytes: The serialized F Prime message to be sent.

        Returns:
            None
        """
        full_serialized_cmd_bytes = b"A5A5 GUI %s" % serialized_cmd_bytes
        self.__send_queue.put(full_serialized_cmd_bytes)

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
            queue_obj = received_serialized_message_queue.get(block=True, timeout=0.5)
            msg_type, _, new_msg_content_bytes = queue_obj
            decoder_dict[msg_type].data_callback(new_msg_content_bytes)
        except queue.Empty:
            pass

    @staticmethod
    def server_recv_task(shutdown_event: multiprocessing.Event,
                         server_address: str,
                         server_port: int,
                         response_port: int,
                         select_timeout: float,
                         data_handler: message_parsing_state_machine.MessageParsingStateMachine,
                         send_queue: multiprocessing.Queue) -> None:
        """A UDP server that receives a connection from a client (presumably the backend) and communicates with it.

        This server feeds the given message parsing state machine with received data, and also pulls bytes to send out
        of the given send queue and then sends them to the client.

        Args:
            shutdown_event: The event to signal the shutdown of this process.
            server_address: The address to which this server will bind.
            server_port: The port to which this server will bind.
            response_port: The port to which data from this server will be sent (the address from the client will be
                           used).
            select_timeout: The timeout of the select call that waits for input on our socket.
            data_handler: The message parsing state machine to be fed with new received data.
            send_queue: The queue from which bytes to be sent to the client will be pulled.

        Returns:
            None
        """
        sock = None
        try:
            signal_utils.setup_signal_handler_for_process(shutdown_event=shutdown_event)

            server_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_sock.bind((server_address, server_port))
            server_sock.settimeout(1)
            client_recv_address = None
            while not shutdown_event.is_set():
                try:
                    # Clear any old messages from the queue before accepting a new client connection
                    while not send_queue.empty():
                        send_queue.get_nowait()

                    while not shutdown_event.is_set():
                        ready = select.select([server_sock], [], [server_sock], select_timeout)
                        if ready[0]:
                            data, client_send_address = server_sock.recvfrom(RECV_MAX_SIZE)
                            client_recv_address = (client_send_address[0], response_port)
                            print("Got {} bytes of data from {}".format(len(data), client_send_address))
                            if len(data) > 0:
                                data_handler.new_bytes(data)
                        elif ready[2]:
                            break

                        try:
                            msg_bytes_to_send = send_queue.get(block=False)

                            assert client_recv_address is not None, \
                                "client_recv_address should always be set when we try to send"

                            result = server_sock.sendto(msg_bytes_to_send, 0, client_recv_address)

                            if result == len(msg_bytes_to_send):
                                print("[server_recv_task]: Send successful")
                            else:
                                print("[server_recv_task]: Send unsuccessful, num sent bytes = {}".format(result))

                        except queue.Empty:
                            pass
                except socket.timeout:
                    pass
        finally:
            if sock:
                sock.close()

            shutdown_event.set()
            print("Server recv task exiting")
