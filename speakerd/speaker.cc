
#include <iostream>
#include <netinet/in.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
/*
 * XXX: ChangeMe when recompiling on other platforms
 * BSD OSS is in sys/soundcard.h
 * Linux uses linux/soundcard.h
 */
#include <sys/soundcard.h>

#include <fdk-aac/aacdecoder_lib.h>

/*
 * Simple music player that decodes AAC files and plays them through Open Sound 
 * System (OSS).
 *
 * Made some assumptions to simplify things a bit:
 *   Using ADTS transport type (TT_MP4_ADTS) typically this corresponds to .aac 
 *   extentions that I found.
 *
 * Output is set to stereo S16
 */

using namespace std;

#define MAX_OUTPUT  (8 * 1024 * 1024)

#define DEFAULT_DSP "/dev/dsp0.0"

int
OpenAndConfigureOSS()
{
    int fd;
    int status;

    fd = open(DEFAULT_DSP, O_WRONLY);
    if (fd < 0) {
        perror("open dsp");
        return -1;
    }

    int fmt = AFMT_S16_NE;
    status = ioctl(fd, SNDCTL_DSP_SETFMT, &fmt);
    if (status < 0) {
        perror("ioctl SETFMT");
        close(fd);
        return -1;
    }

    int chans = 2;
    status = ioctl(fd, SNDCTL_DSP_CHANNELS, &chans);
    if (status < 0) {
        perror("ioctl CHANNELS");
        close(fd);
        return -1;
    }

    int speed = 44100;
    status = ioctl(fd, SNDCTL_DSP_SPEED, &speed);
    if (status < 0) {
        perror("ioctl SPEED");
        close(fd);
        return -1;
    }

    int stereo = 1;
    status = ioctl(fd, SNDCTL_DSP_STEREO, &stereo);
    if (status < 0) {
        perror("ioctl SPEED");
        close(fd);
        return -1;
    }

    return fd;
}

void
DecodeAndPlay(char *buf, unsigned int len, int ossfd)
{
    HANDLE_AACDECODER decoder;
    AAC_DECODER_ERROR status;
    CStreamInfo *info;

    char *outbuf = new char[MAX_OUTPUT];

    decoder = aacDecoder_Open(TT_MP4_ADTS, 1);

    status = aacDecoder_SetParam(decoder, AAC_PCM_MIN_OUTPUT_CHANNELS, 2);
    if (status != AAC_DEC_OK) {
        printf("aacDecoder_SetParam1 Error %x\n", status);
        return;
    }

    status = aacDecoder_SetParam(decoder, AAC_PCM_MAX_OUTPUT_CHANNELS, 2);
    if (status != AAC_DEC_OK) {
        printf("aacDecoder_SetParam2 Error %x\n", status);
        return;
    }

    do {
        unsigned int bytesValid = len;

        unsigned char *bufs[2] = { (unsigned char *)buf, nullptr };
        unsigned int lens[2] = { len, 0 };
        status = aacDecoder_Fill(decoder, bufs, lens, &bytesValid);
        if (status != AAC_DEC_OK) {
            printf("aacDecoder_Fill Error %x\n", status);
            return;
        }

        printf("len: %u, bytesValid: %u\n", len, bytesValid);
        unsigned int usedUp = len - bytesValid;
        buf += usedUp;
        len -= usedUp;

        status = aacDecoder_DecodeFrame(decoder, (INT_PCM *)outbuf, MAX_OUTPUT, 0);
        if (status == AAC_DEC_NOT_ENOUGH_BITS) {
            continue;
        }
        if (status != AAC_DEC_OK) {
            printf("aacDecoder_DecodeFrame Error %x\n", status);
            return;
        }

        info = aacDecoder_GetStreamInfo(decoder);
        if (info->sampleRate != 44100 || info->numChannels != 2) {
            cout << "Music Statistics" << endl;
            cout << "    Sample Rate: " << info->sampleRate << endl;
            cout << "    Channels: " << info->numChannels << endl;
        }

        write(ossfd,
              outbuf,
              info->frameSize * sizeof(short) * info->numChannels);
    } while (len > 0);

    aacDecoder_Close(decoder);
}

/*
int
main(int argc, const char *argv[])
{
    int fd;
    int status;
    struct stat sb;

    if (argc != 2) {
        printf("Usage: %s AACFILE\n", argv[0]);
        return 1;
    }

    status = lstat(argv[1], &sb);
    if (status < 0) {
        perror("lstat");
        return 1;
    }

    int len = sb.st_size;
    char *buf = new char[len];

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    status = read(fd, buf, len);
    if (status < 0) {
        perror("read");
        return 1;
    }
    if (status != len) {
        printf("We didn't read enough bytes!\n");
        return 1;
    }

    int ossfd = OpenAndConfigureOSS();

    printf("DecodeAndPlay: len %d\n", len);
    DecodeAndPlay(buf, len, ossfd);

    close(ossfd);
}
*/

#define BUF_LEN (10 * 1024 * 1024)
char buf[BUF_LEN];
int buflen;


int 
load_song(int client, int msglen)
{
	int offset = 0;
	int status; 
	
	while (offset < msglen) {
		status = read(client, buf + offset, msglen - offset);
		if (status < 0) {
			perror("read");
			printf("Intermediate offset:%d\n", offset);
			return 1;
		}
		offset += status;
	}
	printf("Offset is: %d\n", offset);

	printf("Read data. Status %d\n", status);
	if (offset != msglen) {
		printf("We didn't read enough bytes!\n");
		return 1;
	}

	buflen = msglen;
	printf("Final offset:%d\n", offset);

	return 0;
 }

int
listen_to_commands()
{
    int sock;
    int status;
    int ossfd;
    int reuseaddr = 1;
    int64_t timestamp = 0;


    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock <0) {
	perror("socket");
	return 1;
    }
    
    status = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                        &reuseaddr, sizeof(reuseaddr));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }
    
    status = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
                        &reuseaddr, sizeof(reuseaddr));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
#define PORT (8085)
    server_addr.sin_port = htons(PORT);

    status = ::bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (status <0) {
	perror("bind");
	return 1;
    }

#define BACKLOG_LEN (40)
    status = listen(sock, BACKLOG_LEN);
    if (status <0) {
	perror("listen");
	return 1;
    }

    printf("Listening to port...\n");
    struct sockaddr_in newconn;
    int client;
    socklen_t newconnlen;
    for (;;) {
	newconnlen = sizeof(newconn);
	client = accept(sock, (struct sockaddr *) &newconn, &newconnlen);
	if (status <0) {
		perror("accept");
		continue;
	}
	printf("Accepted connection.\n");

	for (;;) {
		unsigned int poison;
		status = read(client, &poison, sizeof(poison));
		if (status < 0) {
			perror("read");
			return 1;
		}
		if (poison != 0xaa55aa55) {
			printf("Error: Poison value received is %x\n", poison);

		}

		int cmd;
		status = read(client, &cmd, sizeof(cmd));
		if (status < 0) {
			perror("read");
			return 1;
		}
		if (status == 0) {
			printf("Connection closed, breaking out...\n");
			break;
		}

		int arg;
		status = read(client, &arg, sizeof(arg));
		if (status < 0) {
			perror("read");
			return 1;
		}


		printf("Read cmd %d, arg %d\n", cmd, arg);
		switch (cmd) {
			case 1:
				status = load_song(client, arg);

				break;

			case 2:
				// Read current time
				timestamp = 0;
				write(client, &timestamp, sizeof(timestamp));

				break;
			case 3:
				read(client, &timestamp, sizeof(timestamp));
				// Wait until the timestamp time

    				ossfd = OpenAndConfigureOSS();
				DecodeAndPlay(buf, buflen, ossfd);
				printf("DecodeAndPlay: len %d\n", arg);
				close(ossfd);
				break;

			default:
				printf("Invalid command %d\n", cmd);

		}

	}
    }
}

