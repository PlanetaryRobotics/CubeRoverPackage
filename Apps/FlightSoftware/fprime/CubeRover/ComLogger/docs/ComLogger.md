<title>ComLogger Component Dictionary</title>
# ComLogger Component Dictionary


## Command List

|Mnemonic|ID|Description|Arg Name|Arg Type|Comment
|---|---|---|---|---|---|
|CloseFile|0 (0x0)|Forces a close of the currently opened file.| | |
|SendAllLogs|1 (0x1)|Sends all logs from flash to Ground| | |
|SendSetofLogs|2 (0x2)|Sends a set of logs from flash to Ground| | |
| | | |start|U32|The start time (seconds) for a log|
| | | |end|U32|The end time (seconds) for a log|

## Telemetry Channel List

|Channel Name|ID|Type|Description|
|---|---|---|---|
|BYTES_READ|0 (0x0)|U32|Number of bytes read from flash|
|BYTES_WRITTEN|1 (0x1)|U32|Number of bytes written to flash|
|READ_ADDRESS|2 (0x2)|U32|Previous Address Read from flash|
|WRITE_ADDRESS|3 (0x3)|U32|Previous Address written to flash|

## Event List

|Event Name|ID|Description|Arg Name|Arg Type|Arg Size|Description
|---|---|---|---|---|---|---|
|FileOpenError|0 (0x0)|The ComLogger encountered an error opening a file| | | | |
| | | |errornum|U32||The error number returned from open file|
| | | |file|Fw::LogStringArg&|240|The file|
|FileWriteError|1 (0x1)|The ComLogger encountered an error writing to a file| | | | |
| | | |errornum|U32||The error number returned from write file|
| | | |bytesWritten|U32||The number of bytes successfully written to file|
| | | |bytesToWrite|U32||The number of bytes attempted to write to file|
| | | |file|Fw::LogStringArg&|240|The file|
|FileValidationError|2 (0x2)|The ComLogger encountered an error writing the validation file| | | | |
| | | |validationFile|Fw::LogStringArg&|240|The validation file|
| | | |file|Fw::LogStringArg&|240|The file|
| | | |status|U32||The Os::Validate::Status return|
|FileClosed|3 (0x3)|The ComLogger successfully closed a file on command.| | | | |
| | | |file|Fw::LogStringArg&|240|The file|
|TimeNotAvaliable|4 (0x4)|Time sent by ground not currently stored in memory| | | | |
| | | |start|U32||The start time requested by ground|
| | | |end|U32||The end time requested by ground|
|FileReadError|5 (0x5)|The ComLogger encountered an error reading a file| | | | |
| | | |errornum|U32||The error number returned from write file|
| | | |bytesReading|U32||The number of bytes to read|
| | | |file|Fw::LogStringArg&|240|The file|
