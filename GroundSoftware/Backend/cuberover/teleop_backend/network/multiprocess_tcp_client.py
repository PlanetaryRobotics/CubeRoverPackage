import multiprocessing
import select
import socket

from network import base_network_client
from teleop_backend.utils import signal_utils


class MultiprocessTcpClient(base_network_client.BaseNetworkClient):
    """A TCP client for connecting to "Houston" that has its own process for receiving data.

    This class also supports sending data through the same socket that is used for receiving data by the process.

    Attributes:
        RECV_MAX_SIZE: A class variable that indicates the max number of bytes a single recv can accept.
        MAX_SELECT_TIMEOUT: A class variable containing the max allowed timeout for the select call in the receiving
                            task
        __sock: The socket used to send and receive data.
        __recv_proc: The internal receiving process.
    """
    RECV_MAX_SIZE = 4096
    MAX_SELECT_TIMEOUT = 10

    def __init__(self,
                 application_wide_shutdown_event: multiprocessing.Event,
                 select_timeout: float = 0.5):
        """Constructor.

        Args:
            application_wide_shutdown_event: The event used to signal the shutdown of all processes in the application.
            select_timeout: The timeout, in seconds, of the select call in the receiving task.
        """
        super().__init__(application_wide_shutdown_event)
        self.__sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.__sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.__recv_proc = None

        if select_timeout > self.MAX_SELECT_TIMEOUT:
            raise ValueError("The given select timeout ({}) "
                             "is larger than the maximum ({})".format(select_timeout, self.MAX_SELECT_TIMEOUT))

        self.__select_timeout = select_timeout

    def connect(self, address: str, port: int, **kwargs: None):
        """Connects the internal socket to a server with the given address and port, then starts the receiving process.

        Args:
            address: The server IP address.
            port: The server port.
            **kwargs: Any additional keyword arguments. This class does not expect any.

        Returns:
            None
        """
        self.__sock.connect((address, port))
        self.__recv_proc = multiprocessing.Process(target=MultiprocessTcpClient.recv_task,
                                                   name="MultiprocessTcpClient_recv_task",
                                                   args=(self._application_wide_shutdown_event,
                                                         self.__sock,
                                                         self.__select_timeout,
                                                         self._data_handlers))
        self.__recv_proc.start()

    def send(self, data: bytes, **kwargs: int):
        """Synchronously sends all of the given data to the server.

        Args:
            data: The data to be sent to the server.
            **kwargs: Any additional keyword arguments. The only acceptable argument is "flags", which specifies the
                      flags to be passed to socket.sendall(). If "flags" is unspecified, the default value of zero will
                      be used.

        Returns:
            None

        Raises:
            NetworkClientSendError: If any error occurs.
        """
        print("[MultiprocessTcpClient]: Attempting to send {} bytes of data".format(len(data)))
        try:
            result = self.__sock.sendall(data, kwargs.get("flags", 0))
            if result is None:
                print("[MultiprocessTcpClient]: Send successful")
            else:
                print("[MultiprocessTcpClient]: Send unsuccessful, result={}".format(result))
        except OSError as exc:
            raise base_network_client.NetworkClientSendError("Error occurred during send") from exc

    @staticmethod
    def recv_task(app_wide_shutdown_event: multiprocessing.Event,
                  sock: socket.socket,
                  select_timeout: float,
                  data_handlers: list):
        """The receiving task executed by the internal process of this client.

        This task simply receives data and passes it to the data handlers until either the socket is disconnected or one
        of the shutdown flag is set.

        Args:
            app_wide_shutdown_event: An event used to signal a shutdown of all processes in this application
            sock: The pre-connected socket from which this task will receive data.
            select_timeout: The timeout duration of the select call in this task
            data_handlers: The list of data_handlers that will receive the bytes received by the socket.

        Returns:
            None
        """
        try:
            signal_utils.setup_signal_handler_for_process(shutdown_event=app_wide_shutdown_event)

            while not app_wide_shutdown_event.is_set():
                ready = select.select([sock], [], [sock], select_timeout)
                if ready[0]:
                    chunk = sock.recv(MultiprocessTcpClient.RECV_MAX_SIZE)
                    if len(chunk) == 0:
                        print("[MultiprocessTcpClient]: Peer closed the connection")
                        break
                    else:
                        print("[MultiprocessTcpClient]: Got {} bytes of data".format(len(chunk)))

                    for d in data_handlers:
                        d.new_bytes(chunk)

                if ready[2]:
                    break
        finally:
            if sock:
                sock.close()
            app_wide_shutdown_event.set()
            print("[MultiprocessTcpClient]: recv task exiting")
