'''
Created on Saturday, 23 March 2019
@author: raewynd

THIS FILE IS AUTOMATICALLY GENERATED - DO NOT EDIT!!!

XML Source: /home/raewynd/cmu_lunar_robotics/cuberover-teleops-f18/GSWDev/Svc/CmdSequencer/CmdSequencerComponentAi.xml
'''

# Import the types this way so they do not need prefixing for execution.
from models.serialize.type_exceptions import *
from models.serialize.type_base import *

from models.serialize.bool_type import *
from models.serialize.enum_type import *
from models.serialize.f32_type import *
from models.serialize.f64_type import *

from models.serialize.u8_type import *
from models.serialize.u16_type import *
from models.serialize.u32_type import *
from models.serialize.u64_type import *

from models.serialize.i8_type import *
from models.serialize.i16_type import *
from models.serialize.i32_type import *
from models.serialize.i64_type import *

from models.serialize.string_type import *
from models.serialize.serializable_type import *

from models.common import channel_telemetry
# Each file represents the information for a single event
# These module variables are used to instance the channel object within the Gse


COMPONENT = "Svc::CmdSequencer"

NAME = "CS_SequencesCompleted"
ID  = 0x221
CHANNEL_DESCRIPTION = "The number of sequences completed."
TYPE = U32Type()
FORMAT_STRING = None

LOW_RED = None
LOW_ORANGE = None
LOW_YELLOW = None
HIGH_YELLOW = None
HIGH_ORANGE = None
HIGH_RED = None

