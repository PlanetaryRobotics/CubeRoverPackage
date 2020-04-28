import abc
import multiprocessing


class NetworkClientSendError(Exception):
    """An exception raised when an exception occurs while sending data."""
    pass


class BaseNetworkClient(metaclass=abc.ABCMeta):
    """The basic interface of a client used to connect to the server through which rover communication is performed.

    Attributes:
        _data_handlers: A list of objects that at a minimum have a function named "new_bytes" that take a bytes object
                        as its only parameter. Each data handler in the list receives a copy of all bytes received by
                        this client.
        _application_wide_shutdown_event: A multiprocessing.Event used to signal the shutdown of all process that are
                                          part of the teleop backend.
     """

    def __init__(self, application_wide_shutdown_event: multiprocessing.Event):
        """Constructor."""
        self._application_wide_shutdown_event = application_wide_shutdown_event
        self._data_handlers = []

    def register_data_handler(self, handler):
        """Registers an object to be a data handler for this client.

        Each data handler in the list receives a copy of all bytes received by this client.

        Args:
            handler: Any object that has, at a minimum, a method named "new_bytes" that takes a bytes object as its only
                     parameter. This is the method that will be called to give this data handler bytes received by this
                     client.

        Returns:
            None

        """
        self._data_handlers.append(handler)

    @abc.abstractmethod
    def connect(self, address: str, port: int, **kwargs):
        """Connects this client to the server.

        After this function is called, the client will be ready to send data to the server and will be ready to
        receive any data from the server (and feed it to any registered data handlers).

        Args:
            address: The address of the server to which this client should connect.
            port: The port of the server to which this client should connect.
            **kwargs: Any additional keyword arguments, which are specific to each concrete subclass.

        Returns:
            None
        """
        raise NotImplementedError("Concrete subclasses of BaseNetworkClient must implement this function")

    @abc.abstractmethod
    def send(self, data: bytes, **kwargs):
        """Synchronously sends all of the given data to the destination.

        Args:
            data: The data to be sent to the server.
            **kwargs: Any additional keyword arguments, which are specific to each concrete subclass

        Returns:
            None

        Raises:
            NetworkClientSendError: If any error occurs.
        """
        raise NotImplementedError("Concrete subclasses of BaseNetworkClient must implement this function")
