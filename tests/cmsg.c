/*
    Copyright (c) 2014 250bpm s.r.o.  All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "../src/nn.h"
#include "../src/tcp.h"
#include "../src/reqrep.h"

#include "testutil.h"

#define SOCKET_ADDRESS "tcp://127.0.0.1:5555"

int main ()
{
    int rc;
    int rep;
    int req;
    struct nn_msghdr hdr;
    struct nn_iovec iovec;
    unsigned char body [3];
    unsigned char ctrl [256];
    struct nn_cmsghdr *cmsg;
    unsigned char *data;
    void *buf;
    
    rep = test_socket (AF_SP_RAW, NN_REP);
    test_bind (rep, SOCKET_ADDRESS);
    req = test_socket (AF_SP, NN_REQ);
    test_connect (req, SOCKET_ADDRESS);

    /* Test ancillary data in static buffer. */

    test_send (req, "ABC");

    iovec.iov_base = body;
    iovec.iov_len = sizeof (body);
    hdr.msg_iov = &iovec;
    hdr.msg_iovlen = 1;
    hdr.msg_control = ctrl;
    hdr.msg_controllen = sizeof (ctrl);
    rc = nn_recvmsg (rep, &hdr, 0);
    errno_assert (rc == 3);

    cmsg = NN_CMSG_FIRSTHDR (&hdr);
    while (1) {
        nn_assert (cmsg);
        if (cmsg->cmsg_level == PROTO_SP && cmsg->cmsg_type == SP_HDR)
            break;
        cmsg = NN_CMSG_NXTHDR (&hdr, cmsg);
    }
    nn_assert (cmsg->cmsg_len == 8);
    data = NN_CMSG_DATA (cmsg);
    nn_assert (!(data[0] & 0x80));
    nn_assert (data[4] & 0x80);

    rc = nn_sendmsg (rep, &hdr, 0);
    nn_assert (rc == 3);
    test_recv (req, "ABC");

    /* Test ancillary data in dynamically allocated buffer (NN_MSG). */

    test_send (req, "ABC");

    iovec.iov_base = body;
    iovec.iov_len = sizeof (body);
    hdr.msg_iov = &iovec;
    hdr.msg_iovlen = 1;
    hdr.msg_control = &buf;
    hdr.msg_controllen = NN_MSG;
    rc = nn_recvmsg (rep, &hdr, 0);
    errno_assert (rc == 3);

    cmsg = NN_CMSG_FIRSTHDR (&hdr);
    while (1) {
        nn_assert (cmsg);
        if (cmsg->cmsg_level == PROTO_SP && cmsg->cmsg_type == SP_HDR)
            break;
        cmsg = NN_CMSG_NXTHDR (&hdr, cmsg);
    }
    nn_assert (cmsg->cmsg_len == 8);
    data = NN_CMSG_DATA (cmsg);
    nn_assert (!(data[0] & 0x80));
    nn_assert (data[4] & 0x80);

    rc = nn_sendmsg (rep, &hdr, 0);
    nn_assert (rc == 3);
    test_recv (req, "ABC");

    test_close (req);
    test_close (rep);

    return 0;
}

