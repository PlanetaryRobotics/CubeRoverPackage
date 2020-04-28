package com.iris.testserver;

import java.io.IOException;
import java.net.DatagramPacket;

import org.yamcs.ConfigurationException;
import org.yamcs.YConfiguration;
import org.yamcs.commanding.PreparedCommand;
import org.yamcs.tctm.UdpTcDataLink;

public class NameStrippingUdpTcDataLink extends UdpTcDataLink {

    public NameStrippingUdpTcDataLink(String yamcsInstance, 
                                      String name, 
                                      YConfiguration config) throws ConfigurationException {
        super(yamcsInstance, name, config);
    }

    public NameStrippingUdpTcDataLink(String yamcsInstance, String name, String spec) throws ConfigurationException {
        this(yamcsInstance, name, YConfiguration.getConfiguration("udp").getConfig(spec));
    }

    @Override
    public void uplinkCommand(PreparedCommand pc) throws IOException {
        byte[] binary = cmdPostProcessor.process(pc);
        if (binary == null) {
            failedCommand(pc.getCommandId(), "Command does not contain a valid command name");
        } else {
            super.uplinkCommand(pc);
        }
    }
}

