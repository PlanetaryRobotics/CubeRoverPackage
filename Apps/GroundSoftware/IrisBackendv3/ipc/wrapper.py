# -*- coding: utf-8 -*-
"""
Wraps ZeroMQ (ZMQ/0MQ) for IPC connections. All ZMQ functions are wrapped so
that this interface can be changed later if needed to support new requirements
without disrupting the rest of the
system.

Only the interior of this module should have any idea that zmq is being used.
Anything outside of this wrapper should not be affected by the fact that ZMQ is
being used for IPC.

@author: Connor W. Colombo (CMU)
@last-updated: 05/18/2021
"""
from abc import ABC, abstractmethod, abstractclassmethod
from typing import Any, Generic, Optional, Tuple, Union, List, cast, TypeVar
import dataclasses
from enum import Enum

import zmq

from .port import Port
from .topic import Topic
from .settings import settings
from .logging import logger


IPM = TypeVar('IPM')


class InterProcessMessage(Generic[IPM], ABC):
    """
    Interface for any message data which supports being sent between processes.
    """

    @abstractmethod
    def to_ipc_bytes(self) -> bytes:
        """
        Pack this object into bytes to be sent over the IPC network 
        (in a safe way, unlike pickle).
        """
        raise NotImplementedError()

    @abstractclassmethod
    def from_ipc_bytes(cls, data: bytes) -> IPM:
        """
        Unpack bytes sent over IPC to reconstruct the sent object.
        """
        raise NotImplementedError()


class SocketType(Enum):
    """
    Enumerates all possible socket types.
    """
    SERVER = 0x00, zmq.REP
    CLIENT = 0x01, zmq.REQ
    PUBLISHER = 0x02, zmq.PUB
    SUBSCRIBER = 0x03, zmq.SUB

    # Instance attributes for type-checker:
    _zmq_type: int

    def __new__(cls, val: int, zmq_type: int):
        """Constructs a new instance of the Enum."""
        obj = object.__new__(cls)
        obj._value_ = val
        obj._zmq_type = zmq_type

        return obj


def create_context() -> zmq.sugar.context.Context:
    return zmq.Context()


def create_socket(
    context: zmq.sugar.context.Context,
    socket_type: SocketType,
    ports: Union[Port, List[Port]]
) -> zmq.sugar.socket.Socket:
    """
    Creates a socket of the given `socket_type` on the given `context` and binds
    or connects it to the given port/ports.

    Note: servers and publishers are to be bound and so they can only take
    one port. Clients and subscribers can connect to multiple ports.
    """

    if isinstance(ports, Port):
        ports = [ports]
    elif not (isinstance(ports, list) and all(isinstance(p, Port) for p in ports)):
        raise TypeError(
            "`ports` must be a `Port` or a `list` containing only `Port`s. "
            f"instead got: `{ports}` of type `{type(ports)}`."
        )

    # Create socket:
    socket = context.socket(socket_type._zmq_type)

    if socket_type in [SocketType.SERVER, socket_type.PUBLISHER]:
        # Bind Servers and Publishers:
        if len(ports) > 1:
            raise ValueError(
                f"Sockets with type `{socket_type}` are bound and can only take "
                f"one port but were given `{ports}`."
            )

        port = ports[0]
        addr = f"tcp://{settings['ip']}:{port}"
        socket.bind(addr)

        logger.info(  # type: ignore
            f"Created a `{socket_type}` at `{addr}`."
        )

    elif socket_type in [SocketType.CLIENT, socket_type.SUBSCRIBER]:
        # Connect Clients and Subscribers:
        for port in ports:
            socket.connect(f"tcp://{settings['ip']}:{port}")

        logger.info(  # type: ignore
            f"Created a `{socket_type}` connected to `tcp://{settings['ip']}` on ports `{ports}`."
        )

    else:
        raise ValueError(
            f"Unsupported/unimplemented socket type. Got {socket_type}."
        )

    return cast(zmq.sugar.socket.Socket, socket)


def subscribe(socket: zmq.sugar.socket.Socket, topics: Union[Topic, List[Topic]]) -> None:
    """
    Subscribes the given socket to the given topic(s).
    """
    if socket.socket_type != SocketType.SUBSCRIBER._zmq_type:
        raise ValueError(
            f"Attempted to subscribe to topic(s) `{topics}` using a socket "
            f"with invalid type ({socket.socket_type}). "
            f"Only sockets with type {SocketType.SUBSCRIBER} can subscribe to a topic."
        )

    if isinstance(topics, Topic):
        topics = [topics]
    elif not (isinstance(topics, list) and all(isinstance(t, Topic) for t in topics)):
        raise TypeError(
            "`ports` must be a `Topic` or a `list` containing only `Topic`s. "
            f"instead got: `{topics}` of type `{type(topics)}`."
        )

    for topic in topics:
        socket.setsockopt(zmq.SUBSCRIBE, topic.value)
        socket.subscribe(topic)


@dataclasses.dataclass(order=True)
class IpcPayload:
    """
    Data sent over IPC.
    """
    topic_bytes: bytes = b''  # if applicable
    subtopic: bytes = b''  # if applicable
    msg: bytes = b''  # message


def send_to(
    socket: zmq.sugar.socket.Socket,
    data: Union[bytes, InterProcessMessage],
    subtopic: Optional[bytes] = None,
    topic: Optional[Topic] = None
) -> None:
    """
    Sends the given `data` using the given `socket`.
    If a `topic` is given, the data will be published to that `topic` iff the
    `data` is allowed on the given `topic`.

    If `subtopic` bytes are given, they will be prepended to the data. 
    This can be done even if `topic` isn't being used.
    """
    payload = IpcPayload()

    # Add topic if applicable:
    if topic is not None:
        # NOTE: Publishing to a topic just entails leading with the topic bytes.
        # So, anyone (any socket) can do it (not necessary to limit just to `PUBLISHER`s).
        if not topic.allows(data):
            raise ValueError(
                f"Given data `{data!r}` is not allowed on topic `{topic}` "
                f"because its type `{type(data)}` is not a supported type. "
                f"This topic allows: {topic.topic_type}."
            )
        payload.topic_bytes = topic.value

    # Add any subtopic bytes if applicable:
    if subtopic is not None:
        payload.subtopic = subtopic

    # Extract the core data and add it to the message
    if isinstance(data, bytes):
        payload.msg = data
    elif isinstance(data, InterProcessMessage):
        payload.msg = data.to_ipc_bytes()
    else:
        raise ValueError(
            "`data` needs to be `bytes` or an instance of a class which "
            "implements `InterProcessMessage`. "
            f"Instead got `{data}` of type `{type(data)}`."
        )

    socket.send_multipart(dataclasses.astuple(payload))


def read_from(
    socket: zmq.sugar.socket.Socket
) -> IpcPayload:
    raw = socket.recv_multipart()

    if len(raw) != 3:
        logger.warning(
            "Invalid multipart message received. "
            f"Should have three parts but instead it has {len(raw)} part(s). "
            f"Received: {raw}. "
        )
        return IpcPayload()

    return IpcPayload(*raw)
