// http://long.ccaba.upc.edu/long/045Guidelines/eva/ipv6.html
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <iostream>

// some Unix variants don't have IPV6_ADD_MEMBERSHIP/IPV6_DROP_MEMBERSHIP
#if defined(IPV6_JOIN_GROUP) && !defined(IPV6_ADD_MEMBERSHIP)
#define IPV6_ADD_MEMBERSHIP  IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

using namespace std;

int main(int argc, char const *argv[]) {

    struct sockaddr_in6 groupSock;
    int fd = -1;

    char databuf[10];
    int datalen = sizeof databuf;

    /* Create a datagram socket on which to send/receive. */
    if((fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) 
    {
        perror("Opening datagram socket error");
        return 1;
    } 
    else 
    {
        cout << "Opening the datagram socket...OK." << std::endl;
    }

    /* Enable SO_REUSEADDR to allow multiple instances of this */
    /* application to receive copies of the multicast datagrams. */
    int reuse = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof reuse) < 0) 
    {
        perror("Setting SO_REUSEADDR error");
        close(fd);
        return 1;
    }
    else 
    {
        cout << "Setting SO_REUSEADDR...OK." << std::endl;
    }

    /* Initialize the group sockaddr structure with a */
    memset((char *) &groupSock, 0, sizeof groupSock);
    groupSock.sin6_family = AF_INET6;
    // address of the group
    inet_pton(AF_INET6, "ff02::1%lo0", &groupSock.sin6_addr);
    groupSock.sin6_port = htons(4321);

    /* Set local interface for outbound multicast datagrams. */
    /* The IP address specified must be associated with a local, */
    /* multicast capable interface. */
    int ifindex = if_nametoindex ("lo0");
    cout << "ifindex is " << ifindex << std::endl;

    if(setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof ifindex)) {
        perror("Setting local interface error");
        return 1;
    } else {
        cout << "Setting the local interface...OK" << std::endl;
    }

    // choice is 0 for sending and 1 for receiving
    int choice;
    if (argc < 2) {
        cout << "missing argv[1]" << std::endl;
        return 1;
    }
    sscanf (argv[1], "%d", &choice);

    // if sending
    if (choice == 0) {
        memset(databuf, 'a', datalen);
        databuf[sizeof databuf - 1] = '\0';

        if (sendto(fd, databuf, datalen, 0, (sockaddr*)&groupSock, sizeof groupSock) < 0) {
            cout << "Error in send errno=" << errno << std::endl;
        } else {
            cout << "Send okay!" << std::endl;
        }
    }
    // if receiving
    else 
    {
        if (choice == 1) {
            groupSock.sin6_addr = in6addr_any;
            if(bind(fd, (sockaddr*)&groupSock, sizeof groupSock)) {
                perror("Binding datagram socket error");
                close(fd);
                return 1;
            } else {
                cout << "Binding datagram socket...OK." << std::endl;
            }

            /* Join the multicast group ff02::/16 on the local  */
            /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
            /* called for each local interface over which the multicast */
            /* datagrams are to be received. */
            struct ipv6_mreq group;
            inet_pton (AF_INET6, "ff02::1", &group.ipv6mr_multiaddr.s6_addr);
            group.ipv6mr_interface = ifindex;

            if(setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&group, sizeof group) < 0) {
                perror("Adding multicast group error");
                close(fd);
                return 1;
            } else {
                cout << "Adding multicast group...OK." << std::endl;
            }

            if (read(fd, databuf, datalen) < 0) {
                perror("Error in read");
            } else {
                databuf[sizeof databuf - 1] = '\0';// just for safety
                cout << "Read Okay" << std::endl;
                cout << "Message is : " << databuf << std::endl;
            }
        }
    }
    return 0;
}
