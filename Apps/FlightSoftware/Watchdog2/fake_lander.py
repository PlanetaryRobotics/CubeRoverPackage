import serial
import re
import _thread
import struct
import math

keep_running = True
ser = serial.Serial('/dev/ttyUSB0', 9600)  # open serial port

nrx = 0
line = b''
bin_file = 'file.bin'
data_bytes = bytearray(b'')
full_packets = []

# pcap-ng stuff


def save_pcap():
    pcap_fp = open('data.pcapng', 'wb')

    # build the header block
    # 28 bytes since no options
    sh_block = struct.pack('=LLLHHQL', 0x0A0D0D0A, 28, 0x1A2B3C4D, 1, 0,
                           0xFFFFFFFFFFFFFFFF, 28)
    pcap_fp.write(sh_block)  # must be 1st block

    # write IDB
    # 20 bytes since no options
    idb_block = struct.pack('=LLHHLL', 0x00000001, 20, 228, 0, 0, 20)
    pcap_fp.write(idb_block)

    # loop through all the bytes and write them out
    for packet in full_packets:
        packet_len = len(packet)
        packet_len_pad = math.ceil(packet_len/4)*4
        # write out the header
        packet_block_hdr = struct.pack('=LLL', 0x00000003, 16 + packet_len_pad,
                                       packet_len)
        pcap_fp.write(packet_block_hdr)

        # write the packet itself
        pcap_fp.write(packet)

        # pad
        pad = (b'\0') * (packet_len_pad - packet_len)
        if len(pad) > 0:
            pcap_fp.write(pad)

        # footer
        packet_block_ftr = struct.pack('=L', 16 + packet_len_pad)
        pcap_fp.write(packet_block_ftr)

    pcap_fp.close()


def send_dat(dat):
    buf = bytearray(b'')
    buf.append(0xC0)

    for d in dat:
        if d == 0xC0:
            buf.append(0xDB)
            buf.append(0xDC)
        elif d == 0xDB:
            buf.append(0xDB)
            buf.append(0xDD)
        else:
            buf.append(d)

    buf.append(0xC0)
    ser.write(bytes(buf))
    print(buf)


def reader():
    try:
        while True:
            inp = input('> ')
            if inp != '':
                if inp == 'svc':
                    send_dat(bytes.fromhex((
                        '45000027943C400040115735C0A86701C0A86702A410A4100013147A01070000EEFF00C0EC1077'
                    )))
                elif inp == 'dep':
                    send_dat(bytes.fromhex((
                        '45000027E510400040110661C0A86701C0A86702A410A4100013167B01070000EEFF00C0011060'
                    )))
                elif inp == 'hot':  # heater on
                    send_dat(bytes.fromhex((
                        '45000027E510400040110661C0A86701C0A86702A410A4100013167B01070000EEFF00C000101f'
                    )))
                elif inp == 'cold':  # heater off
                    send_dat(bytes.fromhex((
                        '45000027E510400040110661C0A86701C0A86702A410A4100013167B01070000EEFF00C0001020'
                    )))
                elif inp == 'beef':  # set Kp to 0xBEEF
                    send_dat(bytes.fromhex((
                        '450000280001000040115e26deadbeef7f00000100351f90001439cb00080000eeff00c0aa10efbe'
                    )))
                elif inp == 'save':
                    if data_bytes is not None:
                        save_pcap()
                else:
                    send_dat(inp.encode('ascii'))
    except EOFError:
        _thread.interrupt_main()
        _thread.exit()


_thread.start_new_thread(reader, ())

escape = False

while keep_running:
    b = ser.read(1)
    line += b
    b = int.from_bytes(b, 'big')
    if escape:
        if b == 0xDC:
            data_bytes.append(0xC0)
        elif b == 0xDD:
            data_bytes.append(0xDB)
        escape = False
    else:
        if b == 0xC0:
            if len(data_bytes) >= 1:
                full_packets.append(data_bytes)
                data_bytes = bytearray(b'')
            pass
        elif b == 0xDB:
            escape = True
        else:
            data_bytes.append(b)
            #data_bytes.append(bytes(b.hex(), 'utf-8'))

    # print stuff
    print('%02x ' % b, end='', flush=True)
    nrx += 1
    if (nrx % 16) == 0:
        print('')
        #print('    ' + re.sub(r'[^\x00-\x7F]+', '.', line.decode('ascii', 'ignore')))
        line = b''

