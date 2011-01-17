/*
* Copyright (c) 2001, Adam Dunkels.
* Copyright (c) 2009, Joakim Eriksson, Niclas Finne.
* Copyright (c) 2011, STMicroelectronics.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote
*    products derived from this software without specific prior
*    written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This file is part of the uIP TCP/IP stack.
*
* $Id: wpcapslip6.c,v 1.2 2011/01/17 09:16:55 salvopitru Exp $
*/

 /**
 * \file
 *         A driver for the uip6-bridge customized for STM32W that works
 *         under Windows (using cygwin dll). Thanks to Adam Dunkels, Joakim
 *         Eriksson, Niclas Finne for the original code.
 * \author
 *         Salvatore Pitrulli <salvopitru@users.sourceforge.net>
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef __CYGWIN__
#include <alloca.h>
#else /* __CYGWIN__ */
#include <malloc.h>
#endif /* __CYGWIN__ */
#include <signal.h>
//#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
//#include <termios.h>
#include <sys/ioctl.h>
#include <err.h>

//#include <netinet/in.h>
#include <arpa/inet.h>


#include <windows.h>


//#include "net/uip.h"
#include "net/uip_arp.h"





char * wpcap_start(struct uip_eth_addr *addr, int log);

void wpcap_send(void *buf, int len);

uint16_t wpcap_poll(char *buf);

void wpcap_exit(void);

int ssystem(const char *fmt, ...)
__attribute__((__format__ (__printf__, 1, 2)));
void write_to_serial(void *inbuf, int len);

#define PRINTF(...) if(verbose)printf(__VA_ARGS__)

//#define PROGRESS(s) fprintf(stderr, s)
#define PROGRESS(s) do { } while (0)

#define REQUEST_MAC_TIMEOUT 3

typedef enum {
	false = 0,
	true = 1,
} bool;

const char *prog;
/* Local adapter IP address. */
static const char *local_ipaddr = NULL;
/* Attached device's IP address. */
static char rem_ipaddr[INET6_ADDRSTRLEN];
static char *ipprefix = NULL;
static char autoconf_addr[INET6_ADDRSTRLEN] = {0};
static bool autoconf = false;
static bool verbose = false;
static bool tun = false;
static bool clean_addr = false;
static bool clean_route = false;
static bool clean_neighb = false;
static struct uip_eth_addr adapter_eth_addr;
static char * if_name;
OSVERSIONINFO osVersionInfo;

/* Fictitious Ethernet address of the attached device (used in tun mode). */
#define DEV_MAC_ADDR "02-00-00-00-00-02"
static const struct uip_eth_addr dev_eth_addr = {{0x02,0x00,0x00,0x00,0x00,0x02}};


static bool request_mac = true;
static bool send_mac = true;
static bool set_sniffer_mode = true;
static bool set_channel = true;
static bool send_prefix = false;
/* Network prefix for border router. */
const char * br_prefix = NULL;

static int sniffer_mode = 0;
static int channel = 0;

static bool mac_received = false;

int
ssystem(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));

void addAddress(const char * ifname, const char * ipaddr);
void delAddress(const char * ifname, const char * ipaddr);
void addLoWPANRoute(const char * ifname, const char * net, const char * gw);
void delLoWPANRoute(const char * ifname, const char * net);
void addNeighbor(const char * ifname, const char * neighb, const char * neighb_mac);
void delNeighbor(const char * ifname, const char * neighb);
int IPAddrFromPrefix(char * ipaddr, const char * ipprefix, const char * mac);

void print_help()
{
    fprintf(stderr, "usage: %s -s siodev [options] <local interface mac address>\r\n\n", prog);
    fprintf(stderr, "Options:\r\n");
    fprintf(stderr, "-s siodev\tDevice that identifies the bridge or the boder router.\r\n");
    fprintf(stderr, "-B baudrate\tBaudrate of the serial port (default:115200).\r\n");
    fprintf(stderr, " One between:\n");
    fprintf(stderr, "  -a ipaddress/[prefixlen]  The address to be assigned to the local interface.\r\n");
    fprintf(stderr, "\t\tadapter.\r\n");
    fprintf(stderr, "  -p 64bit-prefix   Automatic assignment of the IPv6 address from the specified\r\n");
    fprintf(stderr, "\t\tsubnet prefix, based on bridge's MAC address. It may be\r\n");
    fprintf(stderr, "\t\tfollowed by the prefix length.\r\n");
    fprintf(stderr, "\t\tNot allowed in Border Router mode.\r\n");
    fprintf(stderr, "-c channel\t802.15.4 radio channel.\r\n");
    //fprintf(stderr, "-r\t\tSet sniffer mode. \r\n");
    fprintf(stderr, "-t\t\tUse tun interface, i.e., send bare IP packets to device.\r\n");
    fprintf(stderr, "-b 64bit-prefix\tAttached device is an RPL Border Router (-t option forced).\r\n");
    fprintf(stderr, "\t\t64bit-prefix is the prefix the border router has to announce.\r\n");
    fprintf(stderr, "-v\t\tVerbose. Print more info.\r\n");
    fprintf(stderr, "-h\t\tShow this help.\r\n");
    fprintf(stderr, "\r\n<local interface mac address>\tMAC address of the local interface that will\r\n");
    fprintf(stderr, "\t\tbe used by wcapslip6.\r\n");

    exit(0);
}

int
ssystem(const char *fmt, ...)
{
	char cmd[128];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);
	printf("%s\n", cmd);
	fflush(stdout);
	return system(cmd);
}

int
execProcess(LPDWORD exitCode,const char *fmt, ...)
{
	char cmd[128];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);
	printf("%s\n", cmd);
	fflush(stdout);
	//return system(cmd);

	STARTUPINFO si;
    PROCESS_INFORMATION pi;
	

	ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	// Start the child process. 
    if( !CreateProcess( NULL,   // No module name (use command line)
        cmd,		// Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        //printf( "CreateProcess failed (%d).\n", (int)GetLastError() );

		return -1;
    }

    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

	if(exitCode!=NULL){
		GetExitCodeProcess(pi.hProcess,exitCode);
	}

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

	return 0;

}




#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

/*static void
print_packet(u_int8_t *p, int len) {
    int i;
    printf("\n");
    for(i = 0; i < len; i++) {
        printf("%02x", p[i]);
        if ((i & 3) == 3)
            printf(" ");
        if ((i & 15) == 15)
            printf("\n");
    }
    printf("\n");
}*/

int
is_sensible_string(const unsigned char *s, int len)
{
	int i;
	for(i = 1; i < len; i++) {
		if(s[i] == 0 || s[i] == '\r' || s[i] == '\n' || s[i] == '\t') {
			continue;
		} else if(s[i] < ' ' || '~' < s[i]) {
			return 0;
		}
	}
	return 1;
}


/*
* Read from serial, when we have a packet write it to tun. No output
* buffering, input buffered by stdio.
*/

#define BUF_SIZE 2000

void
serial_to_wpcap(FILE *inslip)
{
	unsigned char buf[BUF_SIZE];

    static int inbufptr = 0;
    int ret;
	unsigned char c;

    unsigned char * inpktbuf;

    if(tun){
        inpktbuf = buf + sizeof(struct uip_eth_hdr);
    }
    else {
        inpktbuf = buf;
    }



read_more:
	if(inbufptr >= BUF_SIZE) {
		inbufptr = 0;
	}
	ret = fread(&c, 1, 1, inslip);

	if(ret == -1) {
		err(1, "serial_to_tun: read");
	}
	if(ret == 0) {
		clearerr(inslip);
		return;
		fprintf(stderr, "serial_to_tun: EOF\n");
		exit(1);
	}
	/*  fprintf(stderr, ".");*/
	switch(c) {
  case SLIP_END:
	  if(inbufptr > 0) {
		  if(inpktbuf[0] == '!') {
			  if (inpktbuf[1] == 'M' && inbufptr == 18) {
				  /* Read gateway MAC address and autoconfigure tap0 interface */
				  char macs[24];
				  int i, pos;
				  for(i = 0, pos = 0; i < 16; i++) {
					  macs[pos++] = inpktbuf[2 + i];
					  if ((i & 1) == 1 && i < 14) {
						  macs[pos++] = ':';
					  }
				  }
				  macs[pos] = '\0';
				  printf("*** Gateway's MAC address: %s\n", macs);
				  mac_received = true;


				  if(autoconf){

                      if(IPAddrFromPrefix(autoconf_addr, ipprefix, macs)!=0){
                          fprintf(stderr, "Invalid IPv6 address.\n");
						  exit(1);
                      }
					  local_ipaddr = autoconf_addr;
					  addAddress(if_name,local_ipaddr);
					  
				  }
                  if(br_prefix != NULL){
                      /* RPL Border Router mode. Add route towards LoWPAN. */

                      if(IPAddrFromPrefix(rem_ipaddr, br_prefix, macs)!=0){
                          fprintf(stderr, "Invalid IPv6 address.\n");
						  exit(1);
                      }

                      addLoWPANRoute(if_name, br_prefix, rem_ipaddr);
                      addNeighbor(if_name, rem_ipaddr, DEV_MAC_ADDR);
                  }

			  }
#define DEBUG_LINE_MARKER '\r'
		  }
		  else if(inpktbuf[0] == '?') {
			   if (inpktbuf[1] == 'M') {
				   /* Send our MAC address. */

				   send_mac = true;
				   set_sniffer_mode = true;
				   set_channel = true;
			   }
               else if (inpktbuf[1] == 'P') {
				   /* Send LoWPAN network prefix to the border router. */
				   send_prefix = true;
			   }
		  }
		  else if(inpktbuf[0] == DEBUG_LINE_MARKER) {
			  fwrite(inpktbuf + 1, inbufptr - 1, 1, stderr);
		  }
		  else if(is_sensible_string(inpktbuf, inbufptr)) {
			  fwrite(inpktbuf, inbufptr, 1, stderr);
		  }
		  else {

              PRINTF("Sending to wpcap\n");

              if(tun){
                  
                  //Ethernet header to be inserted before IP packet
                  struct uip_eth_hdr * eth_hdr = (struct uip_eth_hdr *)buf;

                  memcpy(&eth_hdr->dest, &adapter_eth_addr, sizeof(struct uip_eth_addr));
                  memcpy(&eth_hdr->src, &dev_eth_addr, sizeof(struct uip_eth_addr));

                  eth_hdr->type = htons(UIP_ETHTYPE_IPV6);
                  inbufptr += sizeof(struct uip_eth_hdr);
              }
              //print_packet(inpktbuf, inbufptr);

			  wpcap_send(buf, inbufptr);
			  /*      printf("After sending to wpcap\n");*/
		  }
		  inbufptr = 0;
	  }
	  break;

  case SLIP_ESC:
	  if(fread(&c, 1, 1, inslip) != 1) {
		  clearerr(inslip);
		  /* Put ESC back and give up! */
		  ungetc(SLIP_ESC, inslip);
		  return;
	  }

	  switch(c) {
  case SLIP_ESC_END:
	  c = SLIP_END;
	  break;
  case SLIP_ESC_ESC:
	  c = SLIP_ESC;
	  break;
	  }
	  /* FALLTHROUGH */
  default:
	  inpktbuf[inbufptr++] = c;
	  break;
	}

	goto read_more;
}
/*---------------------------------------------------------------------------*/
unsigned char slip_buf[2000];
int slip_end, slip_begin;
/*---------------------------------------------------------------------------*/

void
slip_send(unsigned char c)
{
	if(slip_end >= sizeof(slip_buf)) {
		err(1, "slip_send overflow");
	}
	slip_buf[slip_end] = c;
	slip_end++;
}
/*---------------------------------------------------------------------------*/
void
slip_send_char(unsigned char c)
{
  switch(c) {
  case SLIP_END:
    slip_send(SLIP_ESC);
    slip_send(SLIP_ESC_END);
    break;
  case SLIP_ESC:
    slip_send(SLIP_ESC);
    slip_send(SLIP_ESC_ESC);
    break;
  default:
    slip_send(c);
    break;
  }
}
/*---------------------------------------------------------------------------*/
int
slip_empty()
{
	return slip_end == 0;
}
/*---------------------------------------------------------------------------*/
void
slip_flushbuf(int fd)
{
	int n;

	if (slip_empty())
		return;

	n = write(fd, slip_buf + slip_begin, (slip_end - slip_begin));

	if(n == -1 && errno != EAGAIN) {
		err(1, "slip_flushbuf write failed");
	} else if(n == -1) {
		PROGRESS("Q");		/* Outqueueis full! */
	} else {
		slip_begin += n;
		if(slip_begin == slip_end) {
			slip_begin = slip_end = 0;
		}
	}
}
/*---------------------------------------------------------------------------*/
void
write_to_serial(void *inbuf, int len)
{
	u_int8_t *p = inbuf;
	int i; /*, ecode;*/

	/*  printf("Got packet of length %d - write SLIP\n", len);*/
	/*  print_packet(p, len);*/

	/* It would be ``nice'' to send a SLIP_END here but it's not
	* really necessary.
	*/
	/* slip_send(outfd, SLIP_END); */
	PRINTF("Writing to serial  len: %d\n", len);
	for(i = 0; i < len; i++) {
		switch(p[i]) {
	case SLIP_END:
		slip_send(SLIP_ESC);
		slip_send(SLIP_ESC_END);
		break;
	case SLIP_ESC:
		slip_send(SLIP_ESC);
		slip_send(SLIP_ESC_ESC);
		break;
	default:
		slip_send(p[i]);
		break;
		}

	}
	slip_send(SLIP_END);
	PROGRESS("t");
}
/*---------------------------------------------------------------------------*/
#ifndef BAUDRATE
#define BAUDRATE B115200
#endif
speed_t b_rate = BAUDRATE;

void
stty_telos(int fd)
{
	struct termios tty;
	speed_t speed = b_rate;
	int i;

	if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");

	if(tcgetattr(fd, &tty) == -1) err(1, "tcgetattr");

	cfmakeraw(&tty);

	/* Nonblocking read. */
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 0;
	tty.c_cflag &= ~CRTSCTS;
	tty.c_cflag &= ~HUPCL;
	tty.c_cflag &= ~CLOCAL;

	cfsetispeed(&tty, speed);
	cfsetospeed(&tty, speed);

	if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

#if 1
	/* Nonblocking read and write. */
	/* if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1) err(1, "fcntl"); */

	tty.c_cflag |= CLOCAL;
	if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

	i = TIOCM_DTR;
	if(ioctl(fd, TIOCMBIS, &i) == -1) err(1, "ioctl");
#endif

	usleep(10*1000);		/* Wait for hardware 10ms. */

	/* Flush input and output buffers. */
	if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");
}

int
devopen(const char *dev, int flags)
{
	char t[32];
	strcpy(t, "/dev/");
	strcat(t, dev);
	return open(t, flags);
}

#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>

int
tun_alloc(char *dev)
{
	struct ifreq ifr;
	int fd, err;

	if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
		return -1;

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	*        IFF_TAP   - TAP device
	*
	*        IFF_NO_PI - Do not provide packet information
	*/
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if(*dev != 0)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
		close(fd);
		return err;
	}
	strcpy(dev, ifr.ifr_name);
	return fd;
}
#else
int
tun_alloc(char *dev)
{
	return devopen(dev, O_RDWR);
}
#endif

//const char *netmask;

void
cleanup(void)
{
	wpcap_exit();
	if(clean_addr){
		delAddress(if_name,local_ipaddr);
	}
    if(clean_route){
        delLoWPANRoute(if_name,br_prefix);        
    }
    if(clean_neighb){
        delNeighbor(if_name, rem_ipaddr);
    }
}

void
sigcleanup(int signo)
{
	fprintf(stderr, "signal %d\n", signo);
	exit(0);			/* exit(0) will call cleanup() */
}

void
sigalarm(int signo)
{
	if(!mac_received){
		fprintf(stderr, "Bridge/Router not found!\n");
		exit(2);
	}
}


void send_commands(void)
{
	char buf[3];

	if (request_mac) {
		slip_send('?');
		slip_send('M');
		slip_send(SLIP_END);

		request_mac = 0;
		alarm(REQUEST_MAC_TIMEOUT);
	}
	/* Send our mac to the device. If it knows our address, it is not needed to change
	the MAC address of our local interface (this can be also unsupported, especially under
	Windows).
	*/
	else if(send_mac && slip_empty()){
		short i;
		PRINTF("Sending our MAC.\n");

		slip_send('!');
		slip_send('M');

		for(i=0; i < 6; i++){
			sprintf(buf,"%02X",adapter_eth_addr.addr[i]);
			slip_send(buf[0]);
			slip_send(buf[1]);
		}
		slip_send(SLIP_END);

		send_mac = false;
	}
	else if(set_sniffer_mode && slip_empty()){

		PRINTF("Setting sniffer mode to %d.\n", sniffer_mode);

		slip_send('!');
		slip_send('O');
		slip_send('S');

		if(sniffer_mode){
			slip_send('1');
		}
		else {
			slip_send('0');
		}
		slip_send(SLIP_END);

		set_sniffer_mode = 0;

	}
	else if(set_channel && slip_empty()){

		if(channel != 0){
			PRINTF("Setting channel %02d.\n", channel);

			slip_send('!');
			slip_send('O');
			slip_send('C');
			
			sprintf(buf,"%02d",channel);
			slip_send(buf[0]);
			slip_send(buf[1]);

			slip_send(SLIP_END);
		}

		set_channel = 0;

	}
    else if(send_prefix && br_prefix != NULL  && slip_empty()){

        struct in6_addr addr;
        int i;

        inet_pton(AF_INET6, br_prefix, &addr);

        fprintf(stderr,"*** Address:%s => %02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
            br_prefix, 
            addr.s6_addr[0], addr.s6_addr[1],
            addr.s6_addr[2], addr.s6_addr[3],
            addr.s6_addr[4], addr.s6_addr[5],
            addr.s6_addr[6], addr.s6_addr[7]);
        slip_send('!');
        slip_send('P');
        for(i = 0; i < 8; i++) {
            /* need to call the slip_send_char for stuffing */
            slip_send_char(addr.s6_addr[i]);
        }
        slip_send(SLIP_END);

        send_prefix = false;

    }
}

void addAddress(const char * ifname, const char * ipaddr)
{
	DWORD exitCode = -1;

	if(osVersionInfo.dwMajorVersion < 6){ // < Windows Vista (i.e., Windows XP; check if this command is ok for Windows Server 2003 too).
		char * substr;
		char tmpaddr[44];

		strncpy(tmpaddr,ipaddr,sizeof(tmpaddr));

		strtok(tmpaddr,"/"); // Remove possible prefix length.
		execProcess(&exitCode,"netsh interface ipv6 add address \"%s\" %s",if_name,tmpaddr);
		substr = strtok(NULL,"/");
		if(substr == NULL){ // A prefix length is not specified.
			// Use a 64 bit prefix
			strcat(tmpaddr,"/64");
			execProcess(NULL,"netsh interface ipv6 add route %s \"%s\"",tmpaddr,if_name);
		}
		else {
			execProcess(NULL,"netsh interface ipv6 add route %s \"%s\"",ipaddr,if_name);
		}
		
	}
	else{
		execProcess(&exitCode,"netsh interface ipv6 add address \"%s\" %s",if_name,ipaddr);
	}
	if(exitCode==0)
		clean_addr = true;	
}

void delAddress(const char * ifname, const char * ipaddr)
{
	char tmpaddr[42];

	strncpy(tmpaddr,ipaddr,sizeof(tmpaddr));
	strtok(tmpaddr,"/"); // Remove possible prefix length.
	
	if(osVersionInfo.dwMajorVersion < 6){ // < Windows Vista (i.e., Windows XP; check if this command is ok for Windows Server 2003 too).
		char * substr;	
		
		execProcess(NULL,"netsh interface ipv6 delete address \"%s\" %s",if_name,tmpaddr);
		substr = strtok(NULL,"/");
		if(substr == NULL){ // A prefix length is not specified.
			// Use a 64 bit prefix
			strcat(tmpaddr,"/64");
			execProcess(NULL,"netsh interface ipv6 delete route %s \"%s\"",tmpaddr,if_name);
		}
		else {
			execProcess(NULL,"netsh interface ipv6 delete route %s \"%s\"",ipaddr,if_name);
		}
		
	}
	else{
		strtok(tmpaddr,"/"); // Remove possible prefix length.
		execProcess(NULL,"netsh interface ipv6 delete address \"%s\" %s",if_name,tmpaddr);
	}
	
}

void addLoWPANRoute(const char * ifname, const char * net, const char * gw)
{
	DWORD exitCode = -1;

	execProcess(&exitCode,"netsh interface ipv6 add route %s/64 \"%s\" %s", net, if_name, gw);
    if(exitCode==0)
        clean_route = true;
}

void delLoWPANRoute(const char * ifname, const char * net)
{
    execProcess(NULL,"netsh interface ipv6 delete route %s/64 \"%s\"", net, if_name);
}

void addNeighbor(const char * ifname, const char * neighb, const char * neighb_mac)
{
	DWORD exitCode = -1;

	if(osVersionInfo.dwMajorVersion < 6){ // < Windows Vista (i.e., Windows XP; check if this command is ok for Windows Server 2003 too).
        
        fprintf(stderr,"Bridge mode only supported on Windows Vista and later OSs.\r\n");
        exit(-1);
		
	}
	else{
        execProcess(&exitCode,"netsh interface ipv6 add neighbor \"%s\" %s \"%s\"", if_name, neighb, neighb_mac);
        if(exitCode==0)
            clean_neighb = true;
	}
}

void delNeighbor(const char * ifname, const char * neighb)
{
    execProcess(NULL,"netsh interface ipv6 delete neighbor \"%s\" %s", if_name, neighb);
}

int IPAddrFromPrefix(char * ipaddr, const char * ipprefix, const char * mac)
{
    struct in6_addr ipv6addr;
    struct uip_802154_longaddr dev_addr;
    char tmp_ipprefix[INET6_ADDRSTRLEN];
    char str_addr[INET6_ADDRSTRLEN] = {0};
    int addr_bytes[8];
    int i;

    strncpy(tmp_ipprefix, ipprefix, INET6_ADDRSTRLEN);

    // sscanf requires int instead of 8-bit for hexadecimal variables.

    sscanf(mac, "%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X",
        &addr_bytes[0],
        &addr_bytes[1],
        &addr_bytes[2],
        &addr_bytes[3],
        &addr_bytes[4],
        &addr_bytes[5],
        &addr_bytes[6],
        &addr_bytes[7]);

    for(i=0;i<8;i++){
        dev_addr.addr[i] = addr_bytes[i];
    }      				  

    /*int i;
    PRINTF("MAC:\n");
    for(i=0; i< 8; i++)
    PRINTF("%02X ",dev_addr.addr[i]);
    PRINTF("\n");*/

    dev_addr.addr[0] |= 0x02;				  

    strtok(tmp_ipprefix,"/");

    if(inet_pton(AF_INET6, tmp_ipprefix, &ipv6addr)!=1){
        return 1;
    }

    // Copy modified EUI-64 to the last 64 bits of IPv6 address.
    memcpy(&ipv6addr.s6_addr[8],&dev_addr,8);

    inet_ntop(AF_INET6,&ipv6addr,str_addr,INET6_ADDRSTRLEN); // To string format.

    char * substr = strtok(NULL,"/");
    if(substr!=NULL){   // Add the prefix length.
        strcat(str_addr,"/");
        strcat(str_addr,substr);
    }
    strcpy(ipaddr, str_addr);

    return 0;
			
}



int
main(int argc, char **argv)
{
	int c;
	int slipfd, maxfd;
	int ret;
	fd_set rset, wset;
	FILE *inslip;
	char siodev[10] = "";
	int baudrate = -2;

	char buf[4000];
    
    prog = argv[0];

	setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

	memset(&osVersionInfo,0,sizeof(OSVERSIONINFO));
	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osVersionInfo);

    while((c = getopt(argc, argv, "B:D:hs:c:ra:p:v:tb:")) != -1) {
		switch (c) {
	case 'B':
		baudrate = atoi(optarg);
		break;

	case 's':
		if(strncmp("/dev/", optarg, 5) == 0) {
			strncpy(siodev,optarg + 5,sizeof(siodev)-1);
		}
		else if(strncmp("COM", optarg, 3) == 0) {

			int portnum;

			portnum = atoi(optarg+3);

			if(portnum == 0){
				err(1,"port number is invalid");
			}
			sprintf(siodev,"ttyS%d",portnum-1);
		}
		else {
			strncpy(siodev,optarg,sizeof(siodev)-1);
		}
		break;

	case 'c':
		channel = atoi(optarg);
		set_channel = 1;
		break;
	case 'r':
		sniffer_mode = 1;
		break;

	case 'a':
		if(autoconf == true){
			print_help();
		}
		local_ipaddr = optarg;
		break;

	case 'p':
		if(local_ipaddr !=NULL){
			print_help();
		}
		autoconf = true;
		ipprefix = optarg;
		break;

	case 'v':
		verbose = true;
		break;

    case 't':
		tun = true;
		break;

    case 'b':
		br_prefix = optarg;
        send_prefix = true;
        send_mac = false;
        tun = true;
		break;

	case '?':
	case 'h':
	default:
		print_help();
		break;
		}
	}
	argc -= (optind - 1);
	argv += (optind - 1);

	if(argc != 2 || *siodev == '\0') {
		print_help();
	}

    if(autoconf == true && br_prefix != NULL){
        fprintf(stderr, "-p and -b options cannot be used together.\r\n");
        print_help();
    }

	sscanf(argv[1],"%2X-%2X-%2X-%2X-%2X-%2X",
		(int *)&adapter_eth_addr.addr[0],(int *)&adapter_eth_addr.addr[1],
        (int *)&adapter_eth_addr.addr[2],(int *)&adapter_eth_addr.addr[3],
        (int *)&adapter_eth_addr.addr[4],(int *)&adapter_eth_addr.addr[5]);
	if_name = wpcap_start(&adapter_eth_addr, verbose);


	if(local_ipaddr!=NULL){
		addAddress(if_name, local_ipaddr);
	}


	switch(baudrate) {
  case -2:
	  break;			/* Use default. */
  case 9600:
	  b_rate = B9600;
	  break;
  case 19200:
	  b_rate = B19200;
	  break;
  case 38400:
	  b_rate = B38400;
	  break;
  case 57600:
	  b_rate = B57600;
	  break;
  case 115200:
	  b_rate = B115200;
	  break;
  default:
	  err(1, "unknown baudrate %d", baudrate);
	  break;
	}



	slipfd = devopen(siodev, O_RDWR | O_NONBLOCK | O_NOCTTY | O_NDELAY | O_DIRECT | O_SYNC );
	if(slipfd == -1) {
		err(1, "can't open siodev ``/dev/%s''", siodev);
	}

	fprintf(stderr, "slip started on ``/dev/%s''\n", siodev);
	stty_telos(slipfd);
	slip_send(SLIP_END);
	inslip = fdopen(slipfd, "r");
	if(inslip == NULL) err(1, "main: fdopen");


	atexit(cleanup);
	signal(SIGHUP, sigcleanup);
	signal(SIGTERM, sigcleanup);
	signal(SIGINT, sigcleanup);
	signal(SIGALRM, sigalarm);

	/* Request mac address from gateway. It may be useful for setting the best 
	IPv6 address of the local interface. */


	while(1) {
		maxfd = 0;
		FD_ZERO(&rset);
		FD_ZERO(&wset);

		send_commands();		

		if(!slip_empty()) {		/* Anything to flush? */
			FD_SET(slipfd, &wset);
		}

		FD_SET(slipfd, &rset);	/* Read from slip ASAP! */
		if(slipfd > maxfd) maxfd = slipfd;
#ifdef WITH_STDIN
		FD_SET(STDIN_FILENO, &rset);  /* Read from stdin too. */
		if(STDIN_FILENO > maxfd) maxfd = STDIN_FILENO;   /* This would not be necessary, since we know STDIN_FILENO is 0. */
#endif

		if(slip_empty()) {
			char *pbuf = buf;

			ret = wpcap_poll(pbuf);
			if( ret > 0 ){
				struct uip_eth_hdr * eth_hdr = (struct uip_eth_hdr *)pbuf;

				if(eth_hdr->type == htons(UIP_ETHTYPE_IPV6)){
					// We forward only IPv6 packet.
                    
                    if(tun){
                        // Cut away ethernet header.
                        pbuf += sizeof(struct uip_eth_hdr);
                        ret -= sizeof(struct uip_eth_hdr);
                    }

					write_to_serial(pbuf, ret);
					/*print_packet(pbuf, ret);*/
					slip_flushbuf(slipfd);
				}
			}
		}
		{
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 10;
			ret = select(maxfd + 1, &rset, &wset, NULL, &tv);
		}
		if(ret == -1 && errno != EINTR) {
			err(1, "select");
		}
		else if(ret > 0) {
			if(FD_ISSET(slipfd, &rset)) {
				/* printf("serial_to_wpcap\n"); */
				serial_to_wpcap(inslip);
				/*	printf("End of serial_to_wpcap\n");*/
			}  

			if(FD_ISSET(slipfd, &wset)) {
				slip_flushbuf(slipfd);				
			}
#ifdef WITH_STDIN
			if(FD_ISSET(STDIN_FILENO, &rset)) {
				char inbuf;
				if(fread(&inbuf,1,1,stdin)){
					if(inbuf=='q'){
						exit(0);
					}
				}
			}
#endif
		}
	}
}
