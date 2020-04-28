package com.iris.testserver;

import java.io.File;
import java.io.IOException;
import java.lang.StringBuilder;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.yamcs.ConfigurationException;
import org.yamcs.YamcsServer;
import org.yamcs.YConfiguration;
import org.yamcs.commanding.PreparedCommand;
import org.yamcs.tctm.GenericCommandPostprocessor;
import org.yamcs.utils.ByteArrayUtils;

public class NameStrippingCommandPostprocessor extends GenericCommandPostprocessor {

    protected List<String> validCommandNames;
    protected List<byte[]> validCommandNamesAsBytes;
    private static final Logger log = LoggerFactory.getLogger(NameStrippingCommandPostprocessor.class);

    public NameStrippingCommandPostprocessor(String yamcsInstance) throws ConfigurationException {
        super(yamcsInstance);
        readValidCommandNames(yamcsInstance, null);
    }

    public NameStrippingCommandPostprocessor(String yamcsInstance, YConfiguration config) 
            throws ConfigurationException {
        super(yamcsInstance, config);
        readValidCommandNames(yamcsInstance, config);
    }

    protected void readValidCommandNames(String yamcsInstance, YConfiguration config) 
            throws ConfigurationException {
        try {
            Path validCommandNameFilePath;

            if (config != null && config.containsKey("validCommandNameFileAbsPath")) {
                validCommandNameFilePath = Paths.get(config.getString("incomingDir"));
            } else {
                Path parent = YamcsServer.getServer().getConfigDirectory();
                validCommandNameFilePath = parent.resolve("validCommandNameFile.txt");
            }
       
            this.validCommandNames = Files.readAllLines(validCommandNameFilePath, Charset.defaultCharset());
            Collections.sort(this.validCommandNames, Comparator.comparingInt(String::length).reversed());

            this.validCommandNamesAsBytes = new ArrayList<byte[]>();
            log.info("Valid command names:");
            for (String name : this.validCommandNames) {
                log.info("    {}", name);
                this.validCommandNamesAsBytes.add(name.getBytes("UTF-8"));
            }
        } catch(IOException ex) {
            throw new ConfigurationException(ex);
        }
    }

    @Deprecated
    public NameStrippingCommandPostprocessor(String yamcsInstance, Map<String, Object> config) {
        this(yamcsInstance, YConfiguration.wrap(config));
    }

    static private boolean isAsciiPrintable(byte ch) {
      return ch >= 32 && ch < 127;
    }

    static private String bytesToPrintableString(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < bytes.length; i++) {
            if (isAsciiPrintable(bytes[i])) {
                sb.append((char) bytes[i]);
            } else {
                sb.append(String.format("\\x%02x", bytes[i]));
            }
        }

        return sb.toString();
      
    }

    static private boolean lengthPrefixedByteArrayPrefixEquals(byte[] byteArray, 
                                                               byte prefix[],
                                                               int numLengthPrefixBytes) {
        if (byteArray.length < prefix.length + numLengthPrefixBytes) {
            return false;
        }

        for (int i = 0; i < prefix.length; i++) {
            // Offset the access into the byteArray by the number of length prefix bytes
            // so that we are comparing the prefix the start of the byteArray *after* the
            // length prefix bytes
            if (byteArray[i + numLengthPrefixBytes] != prefix[i]) {
                return false;
            }
        }

        return true;
    }

    @Override
    public byte[] process(PreparedCommand pc) {
        byte[] binary = pc.getBinary();

        String binaryAsStr = bytesToPrintableString(binary);
        for (byte[] validNameAsBytes : this.validCommandNamesAsBytes) {
            if (lengthPrefixedByteArrayPrefixEquals(binary, validNameAsBytes, 2)) {
                String nameAsStr = bytesToPrintableString(validNameAsBytes);
                log.debug("The command \"{}\" starts with (after the length prefix) the valid command name \"{}\"",
                          binaryAsStr, nameAsStr);
                byte[] trimmedBinary = Arrays.copyOfRange(binary, validNameAsBytes.length + 2, binary.length);
                String trimmedAsStr = bytesToPrintableString(trimmedBinary);
                log.debug("Returning the trimmed binary blob \"{}\"", trimmedAsStr);
                return trimmedBinary;
            }
        }


        log.warn("Received a command with contents \"{}\", but it doesn't start with a valid command name, "
                 + "so it is being ignored", binaryAsStr);

        return null;
    }
}