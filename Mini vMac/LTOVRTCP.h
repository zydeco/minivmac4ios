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

#ifndef use_winsock
#define use_winsock 0
#endif

#if use_winsock
#define my_INVALID_SOCKET INVALID_SOCKET
#define my_SOCKET SOCKET
#define my_closesocket closesocket
#define socklen_t int
#else
#define my_INVALID_SOCKET (-1)
#define my_SOCKET int
#define my_closesocket close
#endif

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
LOCALVAR ui3b tx_buffer[6 + LT_TxBfMxSz] =
"LLpppp";


/*
 Receive buffer for LocalTalk data and its metadata
 */
LOCALVAR unsigned int rx_buffer_allocation = 1800;

LOCALVAR my_SOCKET sock_fd = my_INVALID_SOCKET;
LOCALVAR blnr tcp_ok = falseblnr;

#if use_winsock
LOCALVAR blnr have_winsock = falseblnr;
#endif

LOCALPROC start_tcp(void)
{
#if use_winsock
    WSADATA wsaData;
#endif
    struct sockaddr_in addr;

#if use_winsock
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
#if TCP_dolog
        dbglog_writeln("WSAStartup fails");
#endif
        return;
    }
    have_winsock = trueblnr;
#endif

    if (my_INVALID_SOCKET == (sock_fd =
                              socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))
    {
#if TCP_dolog
        dbglog_writeSockErr("socket");
#endif
        return;
    }

    /* find server from LTOVRTCP_SERVER env, should be in the form 1.2.3.4:12345 */
    char *server = NULL;
    char buf[32];
    short port = 0;
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
        }
    }

    if (port == 0) {
        return;
    }

    /* connect to server */
    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(buf);
    addr.sin_port = htons(port);
#if ! use_winsock
    errno = 0;
#endif
    if (0 != connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr))) {
#if TCP_dolog
        dbglog_writeSockErr("connect");
#endif
        MacMsg("Could not connect to LocalTalk server", strerror(errno), falseblnr);
        return;
    }
#if TCP_dolog
    dbglog_writeln("tcp connected");
#endif

    /* non-blocking I/O is good for the soul */
#if use_winsock
    {
        int iResult;
        u_long iMode = 1;

        iResult = ioctlsocket(sock_fd, FIONBIO, &iMode);
        if (iResult != NO_ERROR) {
            /*
             printf("ioctlsocket failed with error: %ld\n", iResult);
             */
        }
    }
#else
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);
#endif

    tcp_ok = trueblnr;
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
    if (my_INVALID_SOCKET != sock_fd) {
        if (0 != my_closesocket(sock_fd)) {
#if TCP_dolog
            dbglog_writeSockErr("my_closesocket sock_fd");
#endif
        }
    }

#if use_winsock
    if (have_winsock) {
        if (0 != WSACleanup()) {
#if TCP_dolog
            dbglog_writeSockErr("WSACleanup");
#endif
        }
    }
#endif

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
    size_t bytes;
    /* Write the packet to TCP */
#if TCP_dolog
    dbglog_writeln("writing to tcp");
#endif
    embedPacketLength(LT_TxBuffSz + 4);
    embedMyPID();
    if (tcp_ok) {

        bytes = send(sock_fd,
                       (const void *)tx_buffer, LT_TxBuffSz + 6, 0);
#if TCP_dolog
        dbglog_writeCStr("sent ");
        dbglog_writeNum(bytes);
        dbglog_writeCStr(" bytes");
        dbglog_writeReturn();
#endif
        (void) bytes; /* avoid warning about unused */
    }
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
    unsigned char* device_buffer = MyRxBuffer;
    if (tcp_ok == falseblnr)
    {
        return 0;
    }

#if ! use_winsock
    errno = 0;
#endif
    /* peek length */
    ssize_t bytes = recv(sock_fd, (void *)device_buffer, 2, MSG_PEEK);
    if (bytes == 2)
    {
        int incoming_length = (device_buffer[0] << 8) + device_buffer[1];
        bytes = recv(sock_fd, (void*)device_buffer, 2 + incoming_length, MSG_PEEK);
        if (bytes == 2 + incoming_length)
        {
            /* read the packet */
            bytes = recv(sock_fd, (void*)device_buffer, 2 + incoming_length, 0);
        }
    }

    if (bytes < 0) {
#if use_winsock
        if (WSAEWOULDBLOCK != WSAGetLastError())
#else
            if (ECONNRESET == errno || ETIMEDOUT == errno)
            {
                MacMsg("Lost connection to LocalTalk server", strerror(errno), falseblnr);
#if TCP_dolog
                dbglog_writeCStr("tcp error ");
                dbglog_writeCStr(strerror(errno));
                dbglog_writeReturn();
#endif
                tcp_ok = falseblnr;
                my_closesocket(sock_fd);
                sock_fd = my_INVALID_SOCKET;
            }
            else if (EAGAIN != errno)
#endif
            {
#if TCP_dolog
                dbglog_writeCStr("ret");
                dbglog_writeNum(bytes);
                dbglog_writeCStr(", bufsize ");
                dbglog_writeNum(rx_buffer_allocation);
#if ! use_winsock
                dbglog_writeCStr(", errno = ");
                dbglog_writeCStr(strerror(errno));
#endif
                dbglog_writeReturn();
#endif
            }
    } else {
#if TCP_dolog
        dbglog_writeCStr("got ");
        dbglog_writeNum(bytes);
        dbglog_writeCStr(", bufsize ");
        dbglog_writeNum(rx_buffer_allocation);
        dbglog_writeReturn();
#endif
    }
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
            dbglog_writeNum(bytes - 6);
            dbglog_writeCStr(" bytes to receiver");
            dbglog_writeReturn();
#endif
            LT_RxBuffer = MyRxBuffer + 6;
            LT_RxBuffSz = bytes - 6;
        }
    }
}
