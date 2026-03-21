#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

// xv6's ethernet and IP addresses
static uint8 local_mac[ETHADDR_LEN] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static uint32 local_ip = MAKE_IP_ADDR(10, 0, 2, 15);

// qemu host's ethernet address.
static uint8 host_mac[ETHADDR_LEN] = {0x52, 0x55, 0x0a, 0x00, 0x02, 0x02};

// udp packets tracked
static struct udp_packets tracked_packets[MAX_UDP_PACKETS];

static struct spinlock netlock;

// helpers
struct udp *get_udp_from_eth(char *eth_start)
{
	if (eth_start == 0)
		return 0;

	struct eth *eth_hdr = (struct eth *)eth_start;
	// (eth_hdr + 1) points to the memory immediately after the eth struct
	struct ip *ip_hdr = (struct ip *)(eth_hdr + 1);

	// (ip_hdr + 1) points to the memory immediately after the ip struct
	return (struct udp *)(ip_hdr + 1);
}

struct ip *get_ip_from_eth(char *eth_start)
{
	if (eth_start == 0)
		return 0;

	struct eth *eth_hdr = (struct eth *)eth_start;
	// (eth_hdr + 1) points to the memory immediately after the eth struct
	return (struct ip *)(eth_hdr + 1);

	// (ip_hdr + 1) points to the memory immediately after the ip struct
	// return (struct udp *)(ip_hdr + 1);
}

void netinit(void)
{
	initlock(&netlock, "netlock");

	acquire(&netlock);

	memset(tracked_packets, 0, sizeof(tracked_packets));
	for (int i = 0; i < MAX_UDP_PACKETS; i++)
	{
		tracked_packets[i].uport = 0;
		tracked_packets[i].curr_packet = 0;
		memset(tracked_packets[i].upackets, 0, sizeof(tracked_packets[i].upackets));
	}

	release(&netlock);
}

//
// bind(int port)
// prepare to receive UDP packets address to the port,
// i.e. allocate any queues &c needed.
//
uint64
sys_bind(void)
{
	//
	// Your code here.
	//
	//
	acquire(&netlock);

	int port;
	argint(0, &port);

	int i = 0;
	int free_index = -1;
	for (i = 0; i < MAX_UDP_PACKETS; i++)
	{
		int real_index = MAX_UDP_PACKETS - i - 1;
		if (tracked_packets[real_index].uport == 0)
		{
			free_index = real_index;
		}
		else if (tracked_packets[real_index].uport == port)
		{
			free_index = -1;
		}
	}

	if (free_index >= 0)
	{
		tracked_packets[free_index].uport = port;
	}

	release(&netlock);

	return -1;
}

//
// unbind(int port)
// release any resources previously created by bind(port);
// from now on UDP packets addressed to port should be dropped.
//
uint64
sys_unbind(void)
{
	//
	// Optional: Your code here.
	//

	return 0;
}

//
// recv(int dport, int *src, short *sport, char *buf, int maxlen)
// if there's a received UDP packet already queued that was
// addressed to dport, then return it.
// otherwise wait for such a packet.
//
// sets *src to the IP source address.
// sets *sport to the UDP source port.
// copies up to maxlen bytes of UDP payload to buf.
// returns the number of bytes copied,
// and -1 if there was an error.
//
// dport, *src, and *sport are host byte order.
// bind(dport) must previously have been called.
//
uint64
sys_recv(void)
{
	//
	// Your code here.
	//
	struct proc *p = myproc();

	acquire(&netlock);

	int dport;
	uint64 src;
	uint64 sport;
	uint64 buffaddr;
	int len;
	// int w_len;

	argint(0, &dport);
	argaddr(1, &src);
	argaddr(2, &sport);
	argaddr(3, &buffaddr);
	argint(4, &len);

	int total = len + sizeof(struct eth) + sizeof(struct ip) + sizeof(struct udp);
	if (total > PGSIZE)
	{
		release(&netlock);
		return -1;
	}

	// check port
	int bind_index = -1;
	uint8 i = 0;
	for (i = 0; i < MAX_UDP_PACKETS; i++)
	{
		if (tracked_packets[i].uport == dport)
		{
			bind_index = i;
			break;
		}
	}
	if (bind_index < 0)
	{
		release(&netlock);
		return -1;
	}

	uint8 curr_packet = tracked_packets[bind_index].curr_packet;
	i = curr_packet;
	uint8 j = i;
	while (1)
	{
		j = i;

		// take the current packet
		char *packets = tracked_packets[bind_index].upackets[i];
		if (packets != 0)
		{
			/* code */
			struct ip *ip = get_ip_from_eth(packets);
			uint32 ip_src = ntohl(ip->ip_src);

			struct udp *udp = get_udp_from_eth(packets);
			uint16 udp_sport = ntohs(udp->sport);

			printf("arg: %d, len: %d, sof: %ld, total: %d\n",len, udp->ulen, sizeof(udp), total);
			if (copyout(
					p->pagetable,
					src,
					(char *)&ip_src,
					sizeof(ip->ip_src)) < 0 ||
				copyout(
					p->pagetable,
					sport,
					(char *)&udp_sport,
					sizeof(udp->sport)) < 0 ||
				copyout(
					p->pagetable,
					buffaddr,
					(char *)(udp + 1),
					len) < 0)
			{
				printf("recv - src: copyout failed\n");
				kfree(tracked_packets[bind_index].upackets[j]);
				tracked_packets[bind_index].upackets[j] = 0;
				tracked_packets[bind_index].curr_packet = (i + 1) % MAX_UDP_PACKETS;

				release(&netlock);
				return -1;
			}

			i = (i + 1) % MAX_UDP_PACKETS;
			break;
		}

		// check the earliest package
		i = (i + 1) % MAX_UDP_PACKETS;
		if (i == curr_packet)
			break;
	}

	// empty queue, will wait
	if (i == curr_packet)
	{
		tracked_packets[bind_index].curr_packet = 0;

		printf("empty queue\n");
	}
	else
	{
		// reset
		kfree(tracked_packets[bind_index].upackets[j]);
		tracked_packets[bind_index].upackets[j] = 0;
		tracked_packets[bind_index].curr_packet = i;

		release(&netlock);

		return len;
	}

	release(&netlock);

	return -1;
}

// This code is lifted from FreeBSD's ping.c, and is copyright by the Regents
// of the University of California.
static unsigned short
in_cksum(const unsigned char *addr, int len)
{
	int nleft = len;
	const unsigned short *w = (const unsigned short *)addr;
	unsigned int sum = 0;
	unsigned short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1)
	{
		*(unsigned char *)(&answer) = *(const unsigned char *)w;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum & 0xffff) + (sum >> 16);
	sum += (sum >> 16);
	/* guaranteed now that the lower 16 bits of sum are correct */

	answer = ~sum; /* truncate to 16 bits */
	return answer;
}

//
// send(int sport, int dst, int dport, char *buf, int len)
//
uint64
sys_send(void)
{
	struct proc *p = myproc();
	int sport;
	int dst;
	int dport;
	uint64 bufaddr;
	int len;

	argint(0, &sport);
	argint(1, &dst);
	argint(2, &dport);
	argaddr(3, &bufaddr);
	argint(4, &len);

	int total = len + sizeof(struct eth) + sizeof(struct ip) + sizeof(struct udp);
	if (total > PGSIZE)
		return -1;

	char *buf = kalloc();
	if (buf == 0)
	{
		printf("sys_send: kalloc failed\n");
		return -1;
	}
	memset(buf, 0, PGSIZE);

	struct eth *eth = (struct eth *)buf;
	memmove(eth->dhost, host_mac, ETHADDR_LEN);
	memmove(eth->shost, local_mac, ETHADDR_LEN);
	eth->type = htons(ETHTYPE_IP);

	struct ip *ip = (struct ip *)(eth + 1);
	ip->ip_vhl = 0x45; // version 4, header length 4*5
	ip->ip_tos = 0;
	ip->ip_len = htons(sizeof(struct ip) + sizeof(struct udp) + len);
	ip->ip_id = 0;
	ip->ip_off = 0;
	ip->ip_ttl = 100;
	ip->ip_p = IPPROTO_UDP;
	ip->ip_src = htonl(local_ip);
	ip->ip_dst = htonl(dst);
	ip->ip_sum = in_cksum((unsigned char *)ip, sizeof(*ip));

	struct udp *udp = (struct udp *)(ip + 1);
	udp->sport = htons(sport);
	udp->dport = htons(dport);
	udp->ulen = htons(len + sizeof(struct udp));

	char *payload = (char *)(udp + 1);
	if (copyin(p->pagetable, payload, bufaddr, len) < 0)
	{
		kfree(buf);
		printf("send: copyin failed\n");
		return -1;
	}

	e1000_transmit(buf, total);

	return 0;
}

void ip_rx(char *buf, int len)
{
	printf("ll: %d\n", len);
	// don't delete this printf; make grade depends on it.
	static int seen_ip = 0;
	if (seen_ip == 0)
		printf("ip_rx: received an IP packet\n");
	seen_ip = 1;

	acquire(&netlock);

	//
	// Your code here.
	//
	// check if packet port is already bind
	struct eth *eth = (struct eth *)buf;
	struct ip *ip = (struct ip *)(eth + 1);
	struct udp *udp = (struct udp *)(ip + 1);

	// check if protocol is udp
	if (ip->ip_p != IPPROTO_UDP)
	{
		release(&netlock);

		return;
	}

	// check if port is already bind
	int bind_index = -1;
	uint8 i = 0;
	for (i = 0; i < MAX_UDP_PACKETS; i++)
	{
		if (tracked_packets[i].uport == ntohs(udp->dport))
		{
			bind_index = i;
			break;
		}
	}
	if (bind_index < 0)
	{
		printf("\n");
		release(&netlock);

		return;
	}

	// save/drop packets
	// check if queue is full
	i = 0;
	int next_index = -1;
	for (i = 0; i < MAX_UDP_PACKETS; i++)
	{
		struct udp *_udp = get_udp_from_eth(tracked_packets[bind_index].upackets[MAX_UDP_PACKETS - i - 1]);

		if (_udp == 0)
		{
			next_index = MAX_UDP_PACKETS - i - 1;
			// break;
		}
	}
	// drop the packet
	if (next_index < 0)
	{
		kfree(buf);
		release(&netlock);
		return;
	}

	// save the packets
	tracked_packets[bind_index].upackets[next_index] = buf;
	release(&netlock);

	// kfree(buf)
}

//
// send an ARP reply packet to tell qemu to map
// xv6's ip address to its ethernet address.
// this is the bare minimum needed to persuade
// qemu to send IP packets to xv6; the real ARP
// protocol is more complex.
//
void arp_rx(char *inbuf)
{
	static int seen_arp = 0;

	if (seen_arp)
	{
		kfree(inbuf);
		return;
	}
	printf("arp_rx: received an ARP packet\n");
	seen_arp = 1;

	struct eth *ineth = (struct eth *)inbuf;
	struct arp *inarp = (struct arp *)(ineth + 1);

	char *buf = kalloc();
	if (buf == 0)
		panic("send_arp_reply");

	struct eth *eth = (struct eth *)buf;
	memmove(eth->dhost, ineth->shost, ETHADDR_LEN); // ethernet destination = query source
	memmove(eth->shost, local_mac, ETHADDR_LEN);	// ethernet source = xv6's ethernet address
	eth->type = htons(ETHTYPE_ARP);

	struct arp *arp = (struct arp *)(eth + 1);
	arp->hrd = htons(ARP_HRD_ETHER);
	arp->pro = htons(ETHTYPE_IP);
	arp->hln = ETHADDR_LEN;
	arp->pln = sizeof(uint32);
	arp->op = htons(ARP_OP_REPLY);

	memmove(arp->sha, local_mac, ETHADDR_LEN);
	arp->sip = htonl(local_ip);
	memmove(arp->tha, ineth->shost, ETHADDR_LEN);
	arp->tip = inarp->sip;

	e1000_transmit(buf, sizeof(*eth) + sizeof(*arp));

	kfree(inbuf);
}

void net_rx(char *buf, int len)
{
	struct eth *eth = (struct eth *)buf;

	if (len >= sizeof(struct eth) + sizeof(struct arp) &&
		ntohs(eth->type) == ETHTYPE_ARP)
	{
		arp_rx(buf);
	}
	else if (len >= sizeof(struct eth) + sizeof(struct ip) &&
			 ntohs(eth->type) == ETHTYPE_IP)
	{
		ip_rx(buf, len);
	}
	else
	{
		kfree(buf);
	}
}
