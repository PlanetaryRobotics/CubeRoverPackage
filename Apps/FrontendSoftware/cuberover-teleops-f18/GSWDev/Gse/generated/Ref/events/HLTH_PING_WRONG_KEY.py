'''
Created on Saturday, 23 March 2019
@author: raewynd

THIS FILE IS AUTOMATICALLY GENERATED - DO NOT EDIT!!!

XML Source: /home/raewynd/cmu_lunar_robotics/cuberover-teleops-f18/GSWDev/Svc/Health/HealthComponentAi.xml
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

from models.common import event
# Each file represents the information for a single event
# These module variables are used to instance the event object within the Gse


COMPONENT = "Svc::Health"

NAME = "HLTH_PING_WRONG_KEY"
ID  = 0x16b
SEVERITY = "FATAL"
FORMAT_STRING = "Ping entry %s responded with wrong key 0x%08X"
EVENT_DESCRIPTION = "Declare FATAL since task is no longer responding"

# Set arguments list with default values here.
ARGUMENTS = [
    ("entry","The entry passing the warning level",StringType(max_string_len=40)), 
    ("badKey","The incorrect key value",U32Type()), 
    ]
