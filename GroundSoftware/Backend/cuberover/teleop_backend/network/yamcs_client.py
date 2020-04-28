# system imports
import enum
import functools
import multiprocessing
import sys
import typing
import warnings

# yamcs-client imports
import yamcs.client
import yamcs.core.auth
import yamcs.core.futures
from yamcs.tmtc import client as tmtc_client
from yamcs.tmtc import model as tmtc_model

# teleop_backend import
from network import base_network_client

if sys.version_info >= (3, 6):
    COMMAND_STATE_ENUM_BASE_CLASS = enum.IntFlag
else:
    COMMAND_STATE_ENUM_BASE_CLASS = enum.IntEnum


@enum.unique
class CommandState(COMMAND_STATE_ENUM_BASE_CLASS):
    """The conceptual states of a command in the process of sending a command from this client to the lander."""
    NEW_COMMAND = 0
    SENT_TO_YAMCS = 1
    QUEUED_BY_YAMCS = 2
    RELEASED_BY_YAMCS = 3
    SENT_BY_YAMCS = 4
    REJECTED_BY_YAMCS = 5


@enum.unique
class YamcsCommandAckType(enum.Enum):
    """The standard acknowledgement types a YAMCS command."""
    QUEUED = "Acknowledge_Queued"
    RELEASED = "Acknowledge_Released"
    SENT = "Acknowledge_Sent"


@enum.unique
class YamcsCommandAckStatus(enum.Enum):
    """The possible values of the status field of a YAMCS acknowledgement message."""
    OK = "OK"
    NOK = "NOK"


class CommandData:
    """A POD (plain old data) object used to group information about a sent command that is being monitored.

    Attributes:
        state: The current state of the command.
        opcode: The opcode of the command.
        monitored_command: The yamcs-client MonitoredCommand object that represents the in-progress command.

    """
    def __init__(self, initial_state: CommandState, opcode: int, monitored_command: tmtc_model.MonitoredCommand):
        self.state = initial_state
        self.opcode = opcode
        self.monitored_command = monitored_command


class CommandUpdate:
    """A POD (plain old data) object used to represent an update to the state of a command.

    Attributes:
        lookup_id: The lookup ID of the command this update is for.
        opcode: The opcode of the command type of the command this update is for.
        old_state: The state this command is transitioning from.
        new_state: The state this command is transitioning to.
        err_msg_if_rejected: A string error message, only valid if new_state is CommandState.REJECTED_BY_YAMCS
    """
    def __init__(self,
                 lookup_id: int,
                 opcode: int,
                 old_state: CommandState,
                 new_state: CommandState,
                 err_msg_if_rejected: str = None):
        self.lookup_id = lookup_id
        self.opcode = opcode
        self.old_state = old_state
        self.new_state = new_state
        self.err_message_if_rejected = err_msg_if_rejected


class YamcsClient(base_network_client.BaseNetworkClient):
    """A client to communicate with the Astrobotics AMCC YAMCS Server.

    To send a command to the rover, we must first the command to the Astrobotics AMCC YAMCS server. The server expects
    all such commands to be sent as the same YAMCS command. This YAMCS command accepts a single binary argument with the
    argument name specified in `YAMCS_COMMAND_BINARY_ARGUMENT_NAME`. This binary argument shall be populated with a
    binary blob that consists of the F Prime command name in plain text (that is, serialized directly as UTF-8) followed
    by the entire serialized F Prime command. Prior to the mission, Astrobotics will have been supplied with a list of
    all valid command names, so when the server receives a command it will be checked against that list and placed into
    a queue appropriate to the message type (or rejected if the name does not match any on the list). Once on the queue,
    the message may be immediately sent (if the queue requires no validation), but more likely the message will be
    verified by Astrobotics as being safe to perform in the current situation and only then will it be released from
    the queue and sent to the lander. We are able to receive acknowledgements when the command is queued, released from
    the queue, and sent. An initial update is returned by the `send()` function, and all subsequent ones are returned by
    the `get_command_updates()` function. The same process that normally calls the `send()` function should ensure that
    it also calls `get_command_updates()` frequently in order to assure that we can quickly update the database with
    these new command states.

    The Astrobotics AMCC YAMCS server makes all data received from the rover (telemetry, events, and commands) available
    to us via a single YAMCS parameter. Each time new data is received from the rover this parameter, whose value is
    binary blob, has its value set to the new data. This client subscribes for updates to this parameter, so that
    the `__on_parameter_update()` is called each time the parameter is updated. This callback feeds the data to all
    registered data handlers.

    Attributes:
        YAMCS_COMMAND_BINARY_ARGUMENT_NAME: A class variable containing the string name of the single binary argument of
                                            the YAMCS command used to send commands to the server.
        __credentials: The yamcs-client Credentials object that is used to specify the username and password.
        __command_name: The name of the single YAMCS command to which we send all rover commands.
        __client: The yamcs-client YamcsClient object, which represents our connection the YAMCS server at a particular
                  URI, and using `__credentials`.
        __processor: The yamcs-client ProcessorClient object, which is used to interact with a certain processor (e.g.
                     "realtime" of a particular instance (e.g. "Astrobotic-M1") within the YAMCS server.
        __parameter_subscription: The yamcs-client ParameterSubscription object representing the active subscription to
                                  receive updates to the parameter used to access data from the rover.
        __param_name: The name of the YAMCS parameter to which we subscribe.
        __command_connection: A yamcs-client CommandConnection object, which allows us to send commands as
                              MonitoredCommands. With a MonitoredCommand, it is possible to monitor the status of the
                              command as it is queued, released from the queue, and sent (or rejected) by the YAMCS
                              server.
        __lookup_id_to_command_data: A mapping from lookup ID to CommandData object for commands that have been sent to
                                     the YAMCS server by this client, but have not yet been sent or rejected by the
                                     YAMCS server. This is used to track the status commands as they progress through
                                     the YAMCS server.
    """

    YAMCS_COMMAND_BINARY_ARGUMENT_NAME = "tc-binary"

    def __init__(self,
                 yamcs_username: str,
                 yamcs_password: str,
                 yamcs_command_name: str,
                 application_wide_shutdown_event: multiprocessing.Event):
        """Constructor.

        Args:
            yamcs_username: The username of the credentials used to connect to the YAMCS server.
            yamcs_password: The password of the credentials used to connect to the YAMCS server.
            yamcs_command_name: The "name" of the YAMCS command that is sent for all of our commands. The true names of
                                our commands are prepended to the blob of binary data sent as the command argument.
            application_wide_shutdown_event: The event used to signal the shutdown of all processes in the application.
        """
        # Initialize the base class
        super().__init__(application_wide_shutdown_event)

        # Sign in with username and password defined at top of file
        self.__credentials = yamcs.core.auth.Credentials(username=yamcs_username, password=yamcs_password)

        # Store the command "name", which is the same for all commands. The true command name is stored in
        # the binary data blob.
        self.__command_name = yamcs_command_name

        # Define the instance variables that will be constructed when connect() is called.
        self.__client = None
        self.__processor = None
        self.__parameter_subscription = None
        self.__param_name = None
        self.__command_connection = None
        self.__lookup_id_to_command_data = {}  # type: typing.Dict[int, CommandData]

    def __on_parameter_update(self, parameter_data: tmtc_model.ParameterData):
        """A callback that is called each time a parameter is set to a new value.

        All data from the rover is made available to the ground backend via a single YAMCS parameter, where the type of
        the parameter is a blob of binary data.

        Args:
            parameter_data: The new parameter value.

        Returns:
            None
        """
        for param in parameter_data.parameters:
            if param.name == self.__param_name:
                for d in self._data_handlers:
                    d.new_bytes(param.raw_value)

    @staticmethod
    def __fully_qualified_class_name(o: object) -> str:
        """Returns a string containing the fully qualified class name of an object.

        Args:
            o: The object whose class name will be returned.

        Returns:
            The class name.
        """
        module = o.__class__.__module__
        if module is None or module == str.__class__.__module__:
            return o.__class__.__name__  # Avoid reporting __builtin__
        else:
            return module + '.' + o.__class__.__name__

    def __on_subscription_close(self, subscription: yamcs.core.futures.WebSocketSubscriptionFuture):
        """A callback that is called when the subscription closes, due to error or otherwise.

        This callback is used to detect a disconnect from the server and trigger application shutdown.

        Args:
            subscription: The subscription that closed.

        Returns:
            None.
        """
        try:
            exc = subscription.exception(timeout=0.5)
            if exc:
                print("[YamcsClient]: Subscription closed due to \"{}: {}\", so shutting down "
                      "application".format(YamcsClient.__fully_qualified_class_name(exc), exc))
            else:
                print("[YamcsClient]: Subscription closed normally, so shutting down application")
        finally:
            self._application_wide_shutdown_event.set()

    def connect(self, address: str, port: int, **kwargs: str):
        """Connects this client to the YAMCS server, enabling sending and receiving data.

        Args:
            address: The YAMCS server address.
            port: The YAMCS server port.
            **kwargs: Any additional keyword arguments. This class requires three such arguments to be supplied:
                      "yamcs_instance", "yamcs_processor", and "yamcs_feedback_parameter_name". All arguments expect
                      str values. "yamcs_instance" is the name of the YAMCS instance to which this client will connect.
                      "yamcs_processor" is the name of the processor within the YAMCS instance to which commands will be
                      sent and from which parameter data will be received. "yamcs_feedback_parameter_name" is the name
                      of the parameter through which the YAMCS server makes data received from the rover available.

        Returns:
            None

        Raises:
            KeyError: If "yamcs_instance", "yamcs_processor", and "yamcs_feedback_parameter_name" are not all provided
                      as a keyword arguments.
        """
        # Create a yamcs client that connects with our server.
        yamcs_uri = "{}:{}".format(address, port)
        print("[YamcsClient]: Connecting the YAMCS server with URI \"{}\"".format(yamcs_uri))
        self.__client = yamcs.client.YamcsClient(yamcs_uri, credentials=self.__credentials)

        yamcs_instance = kwargs["yamcs_instance"]
        yamcs_processor = kwargs["yamcs_processor"]
        print("[YamcsClient]: Creating a processor client for the \"{}\" processor of the \"{}\" "
              "instance".format(yamcs_processor, yamcs_instance))
        self.__processor = self.__client.get_processor(instance=yamcs_instance,
                                                       processor=yamcs_processor)  # type: tmtc_client.ProcessorClient

        # Work around a bug in yamcs-client package that seems to fail if the first thing we do with a client
        # is a subscription, it fails because the Username/Password haven't been added to the session headers yet.
        # Doing a non-subscription action first populates the session headers, and *then* we can do the subscription.
        # I've created an issue on the yamcs-python Github for this: https://github.com/yamcs/yamcs-python/issues/6
        self.__param_name = kwargs["yamcs_feedback_parameter_name"]
        self.__processor.get_parameter_values([self.__param_name], from_cache=True, timeout=10)

        # Bind this instance to the on_parameter_update function so that we get a functools.partial object that is
        # callable as a free method with a single argument (for the parameter data)
        bound_param_callback_method = functools.partial(YamcsClient.__on_parameter_update, self)

        # Subscribe to the parameter that supplies all data received from the rover
        print("[YamcsClient]: Subscribing to updates of the parameter \"{}\"".format(self.__param_name))
        self.__parameter_subscription = \
            self.__processor.create_parameter_subscription([self.__param_name],
                                                           on_data=bound_param_callback_method,
                                                           send_from_cache=False,
                                                           timeout=5)  # type: tmtc_client.ParameterSubscription

        # Add a callback that happens if the subscription closes
        bound_close_callback_method = functools.partial(YamcsClient.__on_subscription_close, self)
        self.__parameter_subscription.add_done_callback(bound_close_callback_method)

    def send(self, data: bytes, **kwargs: int) -> CommandUpdate:
        """Synchronously sends all of the given data to the YAMCS server.

        All bytes are sent as the contents of a command. It is expected that the name of the command has been prepended
        to the command data in clear text. Note that this synchronously sends the data to the server, but that does not
        mean that the data is synchronously sent to the lander. The `get_command_updates()` function can be used to
        determine if and when the YAMCS server actually sends the command.

        Args:
            data: The data to be sent to the server, including the prepended command name.
            **kwargs: Any additional keyword arguments. This class requires two keyword arguments, "lookup_id" and
                      "opcode". "lookup_id" should be an integer ID used to uniquely identify a command being sent.
                      "opcode" is the identifying number of the message type being sent. Both of these number are needed
                      as they are included in the command update objects returned from this function and
                      `get_command_updates()`

        Returns:
            CommandUpdate: The initial command update.

        Raises:
            NetworkClientSendError: If any error occurs.
        """

        # This subscription needs to happen here because it doesn't work when started by another process than the one
        # that calls this function.
        if self.__command_connection is None:
            # Create a command connection with which we can issue a command and monitor its status
            print("[YamcsClient]: Creating a command connection")
            self.__command_connection = self.__processor.create_command_connection(timeout=5)

        lookup_id = kwargs["lookup_id"]
        opcode = kwargs["opcode"]
        mon_cmd = self.__command_connection.issue(self.__command_name,
                                                  args={
                                                      YamcsClient.YAMCS_COMMAND_BINARY_ARGUMENT_NAME: data.hex()
                                                  })  # type: tmtc_client.MonitoredCommand

        print("[YamcsClient]: Sending command for lookup ID {} to YAMCS server".format(lookup_id))
        self.__lookup_id_to_command_data[lookup_id] = CommandData(initial_state=CommandState.SENT_TO_YAMCS,
                                                                  opcode=opcode,
                                                                  monitored_command=mon_cmd)

        return CommandUpdate(lookup_id=lookup_id,
                             opcode=opcode,
                             old_state=CommandState.NEW_COMMAND,
                             new_state=CommandState.SENT_TO_YAMCS)

    @staticmethod
    def __get_new_state_for_ack_status(acks: typing.Dict[str, tmtc_model.Acknowledgment],
                                       ack_type: YamcsCommandAckType,
                                       new_state_if_ok: CommandState) -> typing.Tuple[CommandState,
                                                                                      typing.Optional[str]]:
        """A helper method to get the new state given the status of a particular acknowledgement type.

        Args:
            acks: All acks currently received for the command in question.
            ack_type: The type of the ack that is having its status checked.
            new_state_if_ok: The new state to use if the status of this ack is OK

        Returns:
            A tuple of the new state and an error string iff the new state is CommandState.REJECTED_BY_YAMCS
            (otherwise None).
        """
        this_ack = acks[ack_type.value]
        this_ack_status = this_ack.status
        assert this_ack_status in [YamcsCommandAckStatus.OK.value, YamcsCommandAckStatus.NOK.value]

        if YamcsCommandAckStatus.OK.value == this_ack_status:
            return new_state_if_ok, None
        elif YamcsCommandAckStatus.NOK.value == this_ack_status:
            warnings.warn("A command received a {} {} ack".format(YamcsCommandAckStatus.NOK.value, ack_type.name))
            return CommandState.REJECTED_BY_YAMCS, this_ack.message

    @staticmethod
    def __check_for_ack_types(acks: typing.Dict[str, tmtc_model.Acknowledgment],
                              received_ack_names: typing.Iterable[str],
                              queued_is_update: bool,
                              released_is_update: bool) -> typing.Tuple[typing.Optional[CommandState],
                                                                        typing.Optional[str]]:
        """A helper method to check some set of ack types to determine if there is a new state and, if so, what it is.

        Args:
            acks: All acks currently received for the command in question.
            received_ack_names: A list of all of the ack names. Equivalent to `acks.keys()`
            queued_is_update: Whether or not a QUEUED ack would be new and therefore cause a new state.
            released_is_update: Whether or not a RELEASED ack would be new and therefore cause a new state.

        Returns:
            A tuple of the new state or None if there is no reason to change state, and an error string iff the
            new state is CommandState.REJECTED_BY_YAMCS (otherwise None).
        """
        # Go in reverse order from latest to earlier ACK, as we'll jump straight to a later state if we've
        # received more than one ack since the last time we checked for an update
        if YamcsCommandAckType.SENT.value in received_ack_names:
            return YamcsClient.__get_new_state_for_ack_status(acks,
                                                              YamcsCommandAckType.SENT,
                                                              CommandState.SENT_BY_YAMCS)
        elif released_is_update and YamcsCommandAckType.RELEASED.value in received_ack_names:
            return YamcsClient.__get_new_state_for_ack_status(acks,
                                                              YamcsCommandAckType.RELEASED,
                                                              CommandState.RELEASED_BY_YAMCS)
        elif queued_is_update and YamcsCommandAckType.QUEUED.value in received_ack_names:
            return YamcsClient.__get_new_state_for_ack_status(acks,
                                                              YamcsCommandAckType.QUEUED,
                                                              CommandState.QUEUED_BY_YAMCS)
        else:
            return None, None

    def get_command_updates(self) -> typing.List[CommandUpdate]:
        """Gets all status updates for all in-flight commands.

        Returns:
            A list of objects that represent the change in state for each command whose state has changed since the
            last time their state was checked.
        """
        cmd_updates = []
        lookup_ids_to_delete_from_dict = []

        for lookup_id, command_data in self.__lookup_id_to_command_data.items():
            # Sanity check on the state of the command. The command state should have been changed from NEW_COMMAND
            # before putting the CommandData object into the __lookup_id_to_command_data dictionary. If the command was
            # SENT_BY_YAMCS or REJECTED_BY_YAMCS then the CommandData object should have been removed from the
            # __lookup_id_to_command_data dictionary
            assert (command_data.state != CommandState.NEW_COMMAND and
                    command_data.state != CommandState.SENT_BY_YAMCS and
                    command_data.state != CommandState.REJECTED_BY_YAMCS)

            # When a command has been sent to YAMCS, we could have received any of the acks, all of which could have
            # had an OK or NOK status.
            acks = command_data.monitored_command.acknowledgments  # type: typing.Dict[str, tmtc_model.Acknowledgment]
            received_ack_names = list(acks.keys())

            # Determine if the state of the command has changed
            if command_data.state == CommandState.SENT_TO_YAMCS:
                new_state, err_msg_if_rejected = YamcsClient.__check_for_ack_types(acks,
                                                                                   received_ack_names,
                                                                                   queued_is_update=True,
                                                                                   released_is_update=True)
            elif command_data.state == CommandState.QUEUED_BY_YAMCS:
                new_state, err_msg_if_rejected = YamcsClient.__check_for_ack_types(acks,
                                                                                   received_ack_names,
                                                                                   queued_is_update=False,
                                                                                   released_is_update=True)
            elif command_data.state == CommandState.RELEASED_BY_YAMCS:
                new_state, err_msg_if_rejected = YamcsClient.__check_for_ack_types(acks,
                                                                                   received_ack_names,
                                                                                   queued_is_update=False,
                                                                                   released_is_update=False)
            else:
                warnings.warn("[YamcsClient]: Unexpected command state: {}".format(command_data.state))
                continue

            # If we don't have an update for this command then just continue to the next one
            if new_state is None:
                continue

            # record the old_state prior to modification
            old_state = command_data.state

            # Update (or remove, if completed) the command data in our dictionary
            if new_state == CommandState.SENT_BY_YAMCS or new_state == CommandState.REJECTED_BY_YAMCS:
                # Need to remove command data from dict, but this will be done after we iterate through in order to
                # avoid issues related to deleting from a dictionary while iterating through it
                lookup_ids_to_delete_from_dict.append(lookup_id)
            else:
                # Update the state of the command data in the dict
                self.__lookup_id_to_command_data[lookup_id].state = new_state

            # Add the new command update object representing this command's state change to our list to be returned
            cmd_updates.append(CommandUpdate(lookup_id,
                                             command_data.opcode,
                                             old_state,
                                             new_state,
                                             err_msg_if_rejected))

        # We're done iterating through the dict, so now delete any mappings from it for commands that have completed
        # sending or have been rejected
        for lookup_id in lookup_ids_to_delete_from_dict:
            del self.__lookup_id_to_command_data[lookup_id]

        return cmd_updates
