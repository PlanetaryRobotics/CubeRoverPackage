SlipEncode__Status SlipEncode__encode(const uint8_t* input,
                                      size_t inputLen,
                                      size_t* inputUsed,
                                      uint8_t* output,
                                      size_t outputLen,
                                      size_t* outputUsed)
{
    size_t outIndex = 0;

    for (size_t inIndex = 0; inIndex < inputLen, ++inIndex) {
        uint8_t inByte = input[inIndex];
        size_t lastByteIndexOffset = (inByte == SLIP_END || inByte == SLIP_ESC) ? 1 : 0;

        if (outIndex + lastByteIndexOffset < outputLen) {
            // There is room for the SLIP-encoded output for this input byte
            if (*in == SLIP_END) {
                output[outIndex] = SLIP_ESC;
                output[outIndex + 1] = SLIP_ESC_END;
                outIndex += 2;
            } else if (*in == SLIP_ESC) {
                output[outIndex] = SLIP_ESC;
                output[outIndex + 1] = SLIP_ESC_ESC;
                outIndex += 2;
            } else {
                output[outIndex] = inByte;
                outIndex++;
            }
        } else {
            *inputUsed = inIndex;
            *outputUsed = outIndex;
             return SLIP_ENCODE__STATUS__OUTPUT_FULL;
        }
    }

    return SLIP_ENCODE__STATUS__INPUT_FINISHED;
}