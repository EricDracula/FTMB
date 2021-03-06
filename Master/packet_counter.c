/*************************************************************************
	> File Name: packet_counter.c
	> Author: Dracula
	> Mail: dracula.guanyu.li@gmail.com
	> Created Time: Saturday, December 02, 2017 PM05:54:59 HKT
 ************************************************************************/

#include "packet_counter.h"

void* packet_counter(void* argv) {
    char err_buf[PCAP_ERRBUF_SIZE];
    pcap_t *src_nic;
    // pcap_t *dst_nic;
    pcap_loop_arg loop_arg;
    // struct bpf_program filter;
    // char filter_str[36];

    packet_counter_arg* pc_arg = (packet_counter_arg*)argv;

    pthread_mutex_lock(pc_arg->mutex);
    *(pc_arg->counter) = 0;
    pthread_mutex_unlock(pc_arg->mutex);

    src_nic = pcap_open_live(SRC_NIC, PACKET_MAX_SIZE, PROMISC_TRIGGER, TO_MS, err_buf);
    // dst_nic = pcap_open_live(DST_NIC, PACKET_MAX_SIZE, PROMISC_TRIGGER, TO_MS, err_buf);

    /* Open src_nic for receiving packet and open dst_nic for sending packet */
    // if (src_nic == NULL || dst_nic == NULL) {
        // fprintf(stderr, "Error: Packet Counter: pcap_open_live(): %s\n", err_buf);
        // exit(2);
    // }
    // else
        // fprintf(stdout,
                // "Packet Counter: Open src_nic %s and dst_nic %s successfully!\n",
                // SRC_NIC, DST_NIC);
    if (src_nic == NULL) {
        fprintf(stderr, "Error: Packet Counter: pcap_open_live(): %s\n", err_buf);
        exit(2);
    }
    else
        fprintf(stdout, "Packet Counter: Open src_nic %s successfully!\n", SRC_NIC);

    /* Filter packets(don't counter packets used for communication among FTMB) */
    // sprintf(filter_str, "icmp or tcp port %d", PRIVATE_PORT);
    // if (pcap_compile(src_nic, &filter, filter_str, OPTIMIZE_TRIGGER, 0) != 0) {
       // fprintf(stderr, "Error: Packet Counter: pcap_compile(): complie filter error\n");
       // exit(3);
    // }
    // if (pcap_setfilter(src_nic, &filter) != 0) {
       // fprintf(stderr, "Error: Packet Counter: pcap_setfilter(): set filter error\n");
       // exit(4);
    // }

    loop_arg.pc_arg = pc_arg;
    // loop_arg.dst_nic = dst_nic;
    pcap_loop(src_nic, PACKET_NUM, get_packet, (u_char*)&loop_arg);

    pcap_close(src_nic);
    // pcap_close(dst_nic);

    return NULL;
}

void get_packet(u_char* arg, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
    packet_counter_arg* pc_arg = ((pcap_loop_arg*)arg)->pc_arg;
    // pcap_t* dst_nic = ((pcap_loop_arg*)arg)->dst_nic;

    struct ether_header* eth_header;

    /* Parse the packet */
    eth_header = (struct ether_header*)packet;
    if (ntohs(eth_header->ether_type) == ETHERTYPE_IP) {
        const u_char* ip_header;
        u_char protocol;
        
        ip_header = packet + ETHER_HEADER_LENGTH;
        protocol = *(ip_header + 9);
        
        if (protocol == IPPROTO_TCP) {
            const u_char* tcp_header;
            unsigned int ip_header_length;
            unsigned int dst_port;
            
            ip_header_length = (*ip_header) & 0x0F;
            ip_header_length = ip_header_length * 4;
            tcp_header = ip_header + ip_header_length;
            dst_port = ntohs(*((uint16_t*)(tcp_header + 2)));
            
            // fprintf(stdout, "Debug: dst_port: %u\n", dst_port);
            if (dst_port == PRIVATE_PORT) {
                const u_char* payload;
                unsigned int tcp_header_length;
                
                tcp_header_length = (((*(tcp_header + 12)) & 0xF0) >> 4) * 4;
                payload = tcp_header + tcp_header_length;
                
                if (*((char*)payload) == 's') {
                    *(pc_arg->nf_process_flag) = 0;
                }
            }
        }
        else if (protocol == IPPROTO_ICMP) {
            /* Just log icmp_echo packet(ping) now */
            const u_char* icmp_header;
            unsigned int ip_header_length;
            
            ip_header_length = (*ip_header) & 0x0F;
            ip_header_length = ip_header_length * 4;
            icmp_header = ip_header + ip_header_length;

            if (*icmp_header == 0) {
                /* ICMP_ECHO */
                pthread_mutex_lock(pc_arg->mutex);
                *(pc_arg->counter) += 1;
                pthread_mutex_unlock(pc_arg->mutex);
            }
            
            // fprintf(stdout, "Debug: packet counter receive 1 icmp packet\n");
        }
        else
            return;
    }
    else
        return;


    /* Send the packet to Master */
    // if (pcap_sendpacket(dst_nic, packet, pkthdr->caplen) != 0) {
       // fprintf(stderr, "Error: Packet Counter: pcap_sendpacket(): send packet error\n");
    // }

}
