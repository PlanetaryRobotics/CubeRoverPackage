package com.iris.testserver;

import java.nio.ByteBuffer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.yamcs.TmPacket;
import org.yamcs.YConfiguration;
import org.yamcs.tctm.PacketPreprocessor;
import org.yamcs.utils.ByteArrayUtils;
import org.yamcs.utils.TimeEncoding;

public class PassThroughPacketPreprocessor implements PacketPreprocessor {

    int sequenceNumber;
    private static final Logger log = LoggerFactory.getLogger(PassThroughPacketPreprocessor.class);

    public PassThroughPacketPreprocessor(String yamcsInstance) {
        this(yamcsInstance, null);
    }

    public PassThroughPacketPreprocessor(String yamcsInstance, YConfiguration config) {
        this.sequenceNumber = 0;
    }

    @Override
    public TmPacket process(TmPacket tmPacket) {
        // Prepend a length-prefix to this packet so we can have a uniform interface between test not test.
        byte[] origPacketBinary = tmPacket.getPacket();
        final ByteBuffer modPacketBuffer = ByteBuffer.allocate(origPacketBinary.length + 2);
        modPacketBuffer.putShort((short) origPacketBinary.length);
        modPacketBuffer.put(origPacketBinary);
        final byte[] newPacket = modPacketBuffer.array();

        TmPacket newTmPacket = new TmPacket(tmPacket.getReceptionTime(), newPacket);
        newTmPacket.setSequenceCount(this.sequenceNumber++);
        newTmPacket.setGenerationTime(TimeEncoding.getWallclockTime());
        newTmPacket.setInvalid(false);

        log.info("Got a packet of size {}, and returning a packet of size {}", 
                 origPacketBinary.length,
                 newPacket.length);

        return newTmPacket;
    }
}

