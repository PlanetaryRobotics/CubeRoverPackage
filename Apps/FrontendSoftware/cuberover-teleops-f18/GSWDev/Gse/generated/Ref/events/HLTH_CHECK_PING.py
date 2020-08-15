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

NAME = "HLTH_CHECK_PING"
ID  = 0x16d
SEVERITY = "ACTIVITY_HI"
FORMAT_STRING = "Health checking set to %s for %s"
EVENT_DESCRIPTION = "Report a particular entry on or off"

# Set arguments list with default values here.
ARGUMENTS = [
    ("enabled","If health pinging is enabled for a particular entry",EnumType("HealthPingIsEnabled",{"HEALTH_PING_DISABLED":0,"HEALTH_PING_ENABLED":1,})), 
    ("entry","The entry passing the warning level",StringType(max_string_len=40)), 
    ]

