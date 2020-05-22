'''
Created on Saturday, 23 March 2019
@author: raewynd

THIS FILE IS AUTOMATICALLY GENERATED - DO NOT EDIT!!!

XML Source: /home/raewynd/cmu_lunar_robotics/cuberover-teleops-f18/GSWDev/Svc/ActiveLogger/ActiveLoggerComponentAi.xml
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


COMPONENT = "Svc::ActiveLogger"

NAME = "ALOG_SEVERITY_FILTER_STATE"
ID  = 0x1a7
SEVERITY = "ACTIVITY_LO"
FORMAT_STRING = "%s filter state. Recv: %d Send: %d"
EVENT_DESCRIPTION = "Dump severity filter state"

# Set arguments list with default values here.
ARGUMENTS = [
    ("severity","The severity level",EnumType("EventFilterState",{"EVENT_FILTER_WARNING_HI":0,"EVENT_FILTER_WARNING_LO":1,"EVENT_FILTER_COMMAND":2,"EVENT_FILTER_ACTIVITY_HI":3,"EVENT_FILTER_ACTIVITY_LO":4,"EVENT_FILTER_DIAGNOSTIC":5,})), 
    ("recvEnabled","",BoolType()), 
    ("sendEnabled","",BoolType()), 
    ]
