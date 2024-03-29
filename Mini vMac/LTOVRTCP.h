/*
 LTOVRTCP.h

 Copyright (C) 2012 Michael Fort, Paul C. Pratt, Rob Mitchelmore

 You can redistribute this file and/or modify it under the terms
 of version 2 of the GNU General Public License as published by
 the Free Software Foundation.  You should have received a copy
 of the license along with this file; see the file COPYING.

 This file is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 license for more details.
 */

/*
 LocalTalk OVeR Transmission Control Protocol
 */

#define TCP_dolog (dbglog_HAVE && 0)

#if TCP_dolog
LOCALPROC dbglog_writeSockErr(char *s)
{
    dbglog_writeCStr(s);
    dbglog_writeCStr(": err ");
#if use_winsock
    dbglog_writeNum(WSAGetLastError());
#else
    dbglog_writeNum(errno);
    dbglog_writeCStr(" (");
    dbglog_writeCStr(strerror(errno));
    dbglog_writeCStr(")");
#endif
    dbglog_writeReturn();
}
#endif

/*
 Transmit buffer for localtalk data and its metadata
 */
LOCALVAR ui3b tx_buffer[6 + LT_TxBfMxSz] = "LLpppp";

/*
 Receive buffer for LocalTalk data and its metadata
 */
LOCALVAR unsigned int rx_buffer_allocation = 1800;

LOCALVAR nw_connection_t connection = nil;
LOCALVAR NSData *nextPacket;

void welcome_next_packet(void)
{
    // schedule receiving length
    nw_connection_receive(connection, 2, 2, ^(dispatch_data_t  _Nullable content, nw_content_context_t  _Nullable context, bool is_complete, nw_error_t  _Nullable error) {
        uint8_t buf[2];
        if (error != nil) {
            NSLog(@"ERROR RECEIVING LENGTH: %@", error);
            return;
        }
        [(NSData*)content getBytes:buf length:2];
        uint16_t length = (buf[0] << 8) | buf[1];

        // schedule receiving packet
        nw_connection_receive(connection, (uint32_t)length, (uint32_t)length, ^(dispatch_data_t  _Nullable content, nw_content_context_t  _Nullable context, bool is_complete, nw_error_t  _Nullable error) {
            if (error != nil) {
                NSLog(@"ERROR RECEIVING DATA: %@", error);
                return;
            }
            nextPacket = (NSData*)content;
        });
    });
}

LOCALPROC start_tcp(void)
{
    /* find server from LTOVRTCP_SERVER env, should be in the form 1.2.3.4:12345 */
    char *server = NULL;
    char buf[32];
    short port = 0;
    char *portStr = NULL;
    if ((server = getenv("LTOVRTCP_SERVER")) && strlen(server) < sizeof(buf)) {
        strcpy(buf, server);
        char *separator = strchr(buf, ':');
        if (separator == NULL) {
            return;
        }
        *separator = 0;
        separator++;
        if (strlen(separator) > 1) {
            port = (short)atoi(separator);
            portStr = separator;
        }
    }

    if (port == 0) {
        return;
    }

    /* connect to server */
    nw_endpoint_t endpoint = nw_endpoint_create_host(buf, portStr);
    nw_parameters_t params = nw_parameters_create_secure_tcp(NW_PARAMETERS_DISABLE_PROTOCOL, ^(nw_protocol_options_t  _Nonnull options) {
        nw_tcp_options_set_no_delay(options, true);
    });
    connection = nw_connection_create(endpoint, params);
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t  _Nullable error) {
        NSLog(@"connection state %@: %@", @[@"invalid", @"waiting", @"preparing", @"ready", @"failed", @"cancelled"][state], error);
    });
    nw_connection_set_queue(connection, dispatch_get_main_queue());
    nw_connection_start(connection);
    welcome_next_packet();
}

LOCALVAR unsigned char *MyRxBuffer = NULL;

/*
 External function needed at startup to initialize the LocalTalk
 functionality.
 */
LOCALFUNC blnr InitLocalTalk(void)
{
    LT_PickStampNodeHint();
    LT_TxBuffer = &tx_buffer[6];

    MyRxBuffer = malloc(rx_buffer_allocation);
    if (NULL == MyRxBuffer) {
        return falseblnr;
    }

    /* Set up TCP socket */
    start_tcp();

    /* Initialized properly */
    return trueblnr;
}

LOCALPROC UnInitLocalTalk(void)
{
    nw_connection_cancel(connection);

    if (NULL != MyRxBuffer) {
        free(MyRxBuffer);
    }
}

LOCALPROC embedPacketLength(ui4r length)
{
    /*
     embeds the length of the packet in the packet as a 16-bit big endian
     */
    tx_buffer[0] = (length >> 8) & 0xff;
    tx_buffer[1] = length & 0xff;
}

LOCALPROC embedMyPID(void)
{
    /*
     embeds my process ID in network byte order in the start of the
     Tx buffer we assume a pid is at most 32 bits.  As far as I know
     there's no actual implementation of POSIX with 64-bit PIDs so we
     should be ok.
     */
    int i;

#if LT_MayHaveEcho
    ui5r v = LT_MyStamp;
#else
    ui5r v = (ui5r)getpid();
#endif

    for (i = 0; i < 4; i++) {
        tx_buffer[2+i] = (v >> (3 - i)*8) & 0xff;
    }
}

GLOBALOSGLUPROC LT_TransmitPacket(void)
{
#if TCP_dolog
    dbglog_writeln("writing to tcp");
#endif
    embedPacketLength(LT_TxBuffSz + 4);
    embedMyPID();
    char *buf = malloc(LT_TxBfMxSz + 6);
    memcpy(buf, tx_buffer, LT_TxBfMxSz + 6);
    dispatch_data_t data = dispatch_data_create(buf, LT_TxBuffSz + 6, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_FREE);
    nw_connection_send(connection, data, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t  _Nullable error) {
        if (error) {
            NSLog(@"nw_connection_send error %@", error);
        }
    });
}

/*
 pidInPacketIsMine returns 1 if the process ID embedded in the packet
 is the same as the process ID of the current process
 */
LOCALFUNC int pidInPacketIsMine(void)
{
    /* is the PID in the packet my own PID? */
    int i;
    ui5r v;

#if LT_MayHaveEcho
    v = LT_MyStamp;
#else
    v = (ui5r)getpid();
#endif

    for (i = 0; i < 4; i++) {
        if (MyRxBuffer[i] != ((v >> (3 - i)*8) & 0xff)) {
            return 0;
        }
    }

    return 1;
}

/*
 packetIsOneISent returns 1 if this looks like a packet that this
 process sent and 0 if it looks like a packet that a different
 process sent.  This provides loopback protection so that we do not
 try to consume packets that we sent ourselves.  We do this by
 checking the process ID embedded in the packet and the IP address
 the packet was sent from.  It would be neater to just look at the
 LocalTalk node ID embedded in the LLAP packet, but this doesn't
 actually work, because during address acquisition it is entirely
 legitimate (and, in the case of collision, *required*) for another
 node to send a packet from what we think is our own node ID.
 */
#if ! LT_MayHaveEcho
LOCALFUNC int packetIsOneISent(void)
{
    return pidInPacketIsMine();
}
#endif

LOCALFUNC int GetNextPacket(void)
{
    if (nextPacket == nil) {
        return 0;
    }

    int bytes = (int)nextPacket.length;
    if (bytes <= rx_buffer_allocation) {
        [nextPacket getBytes:MyRxBuffer length:bytes];
    } else {
        bytes = 0;
    }
    nextPacket = nil;
    welcome_next_packet();
    return bytes;
}

GLOBALOSGLUPROC LT_ReceivePacket(void)
{
    int bytes;

#if ! LT_MayHaveEcho
label_retry:
#endif
    bytes = GetNextPacket();
    if (bytes > 0) {
#if LT_MayHaveEcho
        CertainlyNotMyPacket = ! pidInPacketIsMine();
#endif

#if ! LT_MayHaveEcho
        if (packetIsOneISent()) {
            goto label_retry;
        }
#endif

        {
#if TCP_dolog
            dbglog_writeCStr("passing ");
            dbglog_writeNum(bytes - 4);
            dbglog_writeCStr(" bytes to receiver");
            dbglog_writeReturn();
#endif
            LT_RxBuffer = MyRxBuffer + 4;
            LT_RxBuffSz = bytes - 4;
        }
    }
}
