
#include <iostream>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "../speakerd/printer.h"
#include "../speakerd/timesync.h"

using namespace std;

#define TIMESYNC_PORT 8086

/*
 * Discover a speaker and return the IP address as a string.
 */
TSPkt
Discover_Speaker()
{
    int fd;
    int status;
    struct sockaddr_in addr;
    int reuseaddr = 1;
    int broadcast = 1;

    // Create a network socket
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        abort();
    }

    /*
     * Ensure that we can reopen the address and port without the default 
     * timeout (usually 120 seconds) that blocks reusing addresses and ports 
     * immediately.
     */
    status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                        &reuseaddr, sizeof(reuseaddr));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    status = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                        &reuseaddr, sizeof(reuseaddr));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    // Make this a broadcast socket
    status = setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
                          &broadcast, sizeof(broadcast));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(TIMESYNC_PORT);

    // Bind to the address/port we want to listen to
    status = ::bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0) {
        perror("bind");
        abort();
    }

    while (1) {
        TSPkt pkt;
        ssize_t bufLen = sizeof(pkt);
        struct sockaddr_in srcAddr;
        socklen_t srcAddrLen = sizeof(srcAddr);
        char srcAddrStr[INET_ADDRSTRLEN];

        // Receive a single packet
        bufLen = recvfrom(fd, (void *)&pkt, (size_t)bufLen, 0,
                       (struct sockaddr *)&srcAddr, &srcAddrLen);
        if (bufLen < 0) {
            perror("recvfrom");
            continue;
        }
        if (bufLen != sizeof(pkt)) {
            cout << "Packet recieved with the wrong size!" << endl;
            continue;
        }

        if (pkt.magic != TIMESYNC_MAGIC) {
            cout << "Received a corrupted timesync packet!" << endl;
            continue;
        }

        inet_ntop(AF_INET, &srcAddr.sin_addr, srcAddrStr, INET_ADDRSTRLEN);
        printf("Received from %s\n", srcAddrStr);

        return pkt;
    }
}

int
main(int argc, const char *argv[])
{

	int fd;
	int status;
	struct stat sb;

	if (argc != 2){
		printf("Missing arguments");
		return 1;
	}

	if (lstat(argv[1],&sb) < 0){
		perror("lstat error");
		return 1;
	}

	int len = sb.st_size;
	char *buffer = new char[len];
	
	fd = open(argv[1], O_RDONLY);
	if (fd < 0){
		perror("read error");
		return 1;
	}

	int ttlfilesize = len * sizeof(char);

	int ttlbytesread = 0;

	do{
		int charstoread = ttlfilesize - ttlbytesread;
		
		status = read(fd, buffer+ttlbytesread, charstoread);
		if (status == 0){
			printf("EoF reached");
		}
		if (status < 0){
			perror("read error");
			continue;
		}

		ttlbytesread += status;

		
	}while (ttlbytesread < ttlfilesize);
	
	cout<<"file buffered "<<ttlbytesread<<endl;	

    int speakers[TIMESYNC_MACHINES];
/*
    TSPkt pkt;

    pkt = Discover_Speaker();
cout<<"discovered."<<endl;
    // Read stdin and forward to all speakers
    for (int i = 0; i < TIMESYNC_MACHINES; i++) {
        int status;

        if (pkt.machines[i].ip == 0) {
            speakers[i] = -1;
            continue;
        }

        speakers[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (speakers[i] < 0) {
            perror("socket");
            return 1;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = pkt.machines[i].ip;
        addr.sin_port = htons(MUSICPRINTER_PORT);

        status = connect(speakers[i], (struct sockaddr *)&addr, sizeof(addr));
        if (status < 0) {
            close(speakers[i]);
            speakers[i] = -1;
        }
    }
cout<<"all connected"<<endl;
*/

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, "129.97.75.37", &addr.sin_addr);
	addr.sin_port = htons(MUSICPRINTER_PORT);

	for (int i = 0; i < TIMESYNC_MACHINES; i++){
		speakers[i] = -1;
	}

	speakers[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	status = connect(speakers[0], (struct sockaddr *)&addr, sizeof(addr));
	if (status < 0) {
		perror("connect");
		abort();
	}

	for (int i = 0; i < TIMESYNC_MACHINES; i++){
		int cmd = 1;
		int arg = ttlfilesize;

		cout<<"syncing.."<<endl;
		if (speakers[i] < 0){
			continue;
		}

		write(speakers[i], (char *)&cmd, sizeof(int));
		write(speakers[i], &arg, sizeof(int));


		cout<<sizeof(buffer)<<endl;
		write(speakers[i], buffer, arg); // or use ttlfilesize instead

	}

	printf("reached the end of the code");
}

