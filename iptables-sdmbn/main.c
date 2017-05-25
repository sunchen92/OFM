#include <SDMBN.h>
#include "SDMBNLocal.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack_tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <net/ethernet.h>
#include "utils/network.h"

#define BUFF_SIZE 16384
#define MIN_SIZE 30

static int nl_create_conntrack(struct nf_conntrack *orig);
static int nl_destroy_conntrack(struct nfct_handle *handle, const struct nf_conntrack *ct);


int main(int argc, char **argv)
{
    char *interface = NULL;

    // Parse command line options
    char option = 0;
    while ((option = getopt(argc, argv, "i:")) > 0)
    {
        switch (option)
        {
        case 'i':
            interface = optarg;
            break;
        default:
            return 1;
        }
    }

    // Initialize SDMBN
    SDMBNLocals locals;
    bzero(&locals, sizeof(locals));
    locals.get_perflow = &local_get_perflow;
    locals.put_perflow = &local_put_perflow;
    locals.get_config = &local_get_config;
    locals.put_config = &local_put_config;
	locals.delete_perflow = &local_del_perflow;
    locals.device = interface;
    sdmbn_init(&locals);
    //printf("Testing...\n");
    //PerflowKey key;
    //key.dl_type = ETHERTYPE_IP;
    //key.wildcards = WILDCARD_ALL & ~WILDCARD_DL_TYPE;
    //local_get_perflow(&key, 1);
	//local_del_perflow(&key,1);
	//local_put_perflow(30001, &key, state);
	//local_get_config(1);
	//local_put_config(30001, state);
    // Continue running until user decides to quit
    printf("Running...\n");
    printf("Enter 'q' to quit\n");
    while (getchar() != 'q') ;

    // Clean-up SDMBN
    sdmbn_cleanup();

    return 0;
}


int get_count;
// Collects the data from conntrack queries
int conntrack_get_callback(enum nf_conntrack_msg_type type, 
        struct nf_conntrack *ct, void *data)
{
	struct get_perflow_args args;
	args = *((struct get_perflow_args *)data);
    PerflowKey *key = args.key;
    int id = args.id;
	PerflowKey connkey;
	connkey.wildcards = WILDCARD_ALL;
	if (!(key->wildcards & WILDCARD_TP_SRC))
	{ 
		if (!nfct_attr_is_set(ct, ATTR_PORT_SRC))
        	return NFCT_CB_CONTINUE;
		uint16_t s_port = nfct_get_attr_u16(ct, ATTR_PORT_SRC);
		if ( s_port != key->tp_src)
			return NFCT_CB_CONTINUE;
		connkey.tp_src = s_port;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_TP_SRC;
	}
	if (!(key->wildcards & WILDCARD_TP_DST))
	{ 
		if (!nfct_attr_is_set(ct, ATTR_PORT_DST))
        	return NFCT_CB_CONTINUE;
		uint16_t d_port = nfct_get_attr_u16(ct, ATTR_PORT_DST);
		if ( d_port != key->tp_dst)
			return NFCT_CB_CONTINUE;
		connkey.tp_dst = d_port;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_TP_DST;
	}

	struct nethdr *net;
	net = BUILD_NETMSG(ct, NET_T_STATE_NEW);
	char *state = sdmbn_base64_encode(net, ntohs(net->len));
	
	if (NULL == state) return NFCT_CB_CONTINUE;

	if (nfct_get_attr_u8(ct, ATTR_L3PROTO) == AF_INET) {
		connkey.nw_src = nfct_get_attr_u32(ct, ATTR_IPV4_SRC);
		connkey.nw_src_mask = 32;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_SRC;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_SRC_MASK;
		connkey.nw_dst = nfct_get_attr_u32(ct, ATTR_IPV4_DST);
		connkey.nw_dst_mask = 32;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_DST;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_DST_MASK;
	    connkey.dl_type = ETHERTYPE_IP;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_DL_TYPE;
	}
	else if (nfct_get_attr_u8(ct, ATTR_L3PROTO) == AF_INET6) {
		//TODO: IPv6
		//connkey.nw_src = *((int *)nfct_get_attr(ct, ATTR_IPV6_SRC));
		//connkey.nw_src = nfct_get_attr_u32(ct, ATTR_IPV6_SRC);		
		//connkey.nw_src_mask = 128;
		//connkey.nw_dst = nfct_get_attr_u32(ct, ATTR_IPV6_DST);
		//connkey.nw_dst_mask = 128;
	    //connkey.dl_type = ETHERTYPE_IPV6;
	}
    connkey.nw_proto = nfct_get_attr_u16(ct, ATTR_L4PROTO);
	connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_PROTO;
	connkey.wildcards = connkey.wildcards & ~WILDCARD_DL_TYPE;
    //connkey.wildcards = WILDCARD_NONE;
	uint32_t hashkey = nfct_get_attr_u32(ct, ATTR_ID);
	//printf("connkey.tp_src = %d\n", connkey.tp_src);
	//printf("connkey.tp_dst = %d\n", connkey.tp_dst);
	//printf("connkey.nw_src = %d\n", connkey.nw_src);
	//printf("connkey.nw_dst = %d\n", connkey.nw_dst);
	//printf("hashkey = %d\n", hashkey);
	//printf("get_count = %d\n",get_count);
	
	int result = sdmbn_send_perflow(id, &connkey, state, hashkey, get_count);

    // Increment count
    get_count++;
    // Clean-up
    free(state);

    return NFCT_CB_CONTINUE;
}

//int get_count;
// Collects the data from conntrack queries
int send_state() 
{
    	int id = 1;
	PerflowKey connkey;
	connkey.wildcards = WILDCARD_ALL;

	connkey.tp_src = 1111;
	connkey.wildcards = connkey.wildcards & ~WILDCARD_TP_SRC;

	struct nethdr *net;
	char *state = "lalala\0";
	

		connkey.nw_src = 123456;
		connkey.nw_src_mask = 32;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_SRC;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_SRC_MASK;
		connkey.nw_dst = 875897;
		connkey.nw_dst_mask = 32;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_DST;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_DST_MASK;
	    connkey.dl_type = ETHERTYPE_IP;
		connkey.wildcards = connkey.wildcards & ~WILDCARD_DL_TYPE;
    connkey.nw_proto = 0x06;
	connkey.wildcards = connkey.wildcards & ~WILDCARD_NW_PROTO;
	connkey.wildcards = connkey.wildcards & ~WILDCARD_DL_TYPE;
    //connkey.wildcards = WILDCARD_NONE;
	uint32_t hashkey = 0x019294;
	//printf("connkey.tp_src = %d\n", connkey.tp_src);
	//printf("connkey.tp_dst = %d\n", connkey.tp_dst);
	//printf("connkey.nw_src = %d\n", connkey.nw_src);
	//printf("connkey.nw_dst = %d\n", connkey.nw_dst);
	//printf("hashkey = %d\n", hashkey);
	//printf("get_count = %d\n",get_count);
	
	get_count = 1;
	int i = 0;
	for(i = 0; i <=8; i++){
		int result = sdmbn_send_perflow(id, &connkey, state, hashkey, get_count);
    // Increment count
   		 get_count++;
	}
    // Clean-up
    //free(state);

    return NFCT_CB_CONTINUE;
}
///// SDMBN Local Perflow State Handlers /////////////////////////////////////
int local_get_perflow(PerflowKey *key, int id, int raiseEvents)
{
    if (NULL == key)
    { return -1; }
	
    get_count = 0;

    // Open a ctnetlink handler
    struct nfct_handle *handle = nfct_open(CONNTRACK, 0);
    if (NULL == handle)
    { 
        fprintf(stderr, "nfct_open FAILED: %s\n", strerror(errno));
        return -1;  
    }

    // Construct a filter 
    struct nfct_filter *filter = nfct_filter_create();
    if (NULL == filter)
    {
        fprintf(stderr, "nfct_filter_create FAILED: %s\n", strerror(errno));
        return -1;
    }
	
    // Determine dl_type to query
    u_int8_t family = -1;
    if (!(key->wildcards & WILDCARD_DL_TYPE))
    {
        switch(key->dl_type)
        {
        case ETHERTYPE_IP:
            family = AF_INET;
            break;  
        case ETHERTYPE_IPV6:
            family = AF_INET6;
            break;
		default:
			printf("protocol family not supported. Get failed\n");
			return -1;
        }
    }	
	
    // Add nw_proto to filter
    if (!(key->wildcards & WILDCARD_NW_PROTO))
    { nfct_filter_add_attr_u32(filter, NFCT_FILTER_L4PROTO, key->nw_proto); }
	
	if (family==AF_INET)
	{
		//IPv4
		// Add nw_src to filter
		if (!(key->wildcards & WILDCARD_NW_SRC))
		{
		    struct nfct_filter_ipv4 filterIP;
		    filterIP.addr = ntohl(key->nw_src);
		    filterIP.mask = 0xFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIP.mask = filterIP.mask << (32 - key->nw_src_mask);
			}		    
			nfct_filter_add_attr(filter, NFCT_FILTER_SRC_IPV4, &filterIP);
		}

		// Add nw_dst to filter
		if (!(key->wildcards & WILDCARD_NW_DST))
		{
		    struct nfct_filter_ipv4 filterIP;
		    filterIP.addr = ntohl(key->nw_dst);
		    filterIP.mask = 0xFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIP.mask = filterIP.mask << (32 - key->nw_src_mask);
			}
		    nfct_filter_add_attr(filter, NFCT_FILTER_DST_IPV4, &filterIP);
		}
	}
	else if (family==AF_INET6)
	{
		//TODO: IPv6 - not supported currently
		// Add nw_src to filter
		/*if (!(key->wildcards & WILDCARD_NW_SRC))
		{
		    struct nfct_filter_ipv6 filterIPv6;
		    filterIPv6.addr = key->nw_src;
		    filterIPv6.mask = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIPv6.mask = filterIPv6.mask << (128 - key->nw_src_mask);
			}		    
			nfct_filter_add_attr(filter, NFCT_FILTER_SRC_IPV6, &filterIPv6);
		}

		// Add nw_dst to filter
		if (!(key->wildcards & WILDCARD_NW_DST))
		{
		    struct nfct_filter_ipv6 filterIPv6;
		    filterIPv6.addr = key->nw_dst;
			filterIPv6.mask = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIPv6.mask = filterIPv6.mask << (128 - key->nw_src_mask);
			}
		    nfct_filter_add_attr(filter, NFCT_FILTER_DST_IPV6, &filterIPv6);
		}*/
	}

    // Attach the filter to the handle
    if (nfct_filter_attach(nfct_fd(handle), filter) < 0)
    {
        fprintf(stderr, "nfct_filter_attach FAILED: %s\n", strerror(errno));
        return -1;
    }

    // Register call back
    struct get_perflow_args args;
    args.key = key;
    args.id = id;
    nfct_callback_register(handle, NFCT_T_ALL, conntrack_get_callback, &args);

    // Issue query
    if (nfct_query(handle, NFCT_Q_DUMP, &family) < 0)
    {
        fprintf(stderr,"nfct_query FAILED: %s\n", strerror(errno));
        return -1;
    }
	printf("Sending states ........\n");
	int _ret = send_state();
	printf("Send state result %u\n", _ret);    
// Clean-up
    nfct_filter_destroy(filter);
    nfct_close(handle);

    return get_count;
}

struct nfct_handle *h;
static int nl_destroy_conntrack(struct nfct_handle *handle, const struct nf_conntrack *ct)
{
    return nfct_query(handle, NFCT_Q_DESTROY, ct);
}

static int nl_create_conntrack(struct nf_conntrack *orig)
{
	int ret;
    struct nf_conntrack *ct;

	ct = nfct_clone(orig);
	if (ct == NULL)
		return -1;

	// we hit error if we try to change the expected bit
	if (nfct_attr_is_set(ct, ATTR_STATUS)) {
		uint32_t status = nfct_get_attr_u32(ct, ATTR_STATUS);
		status &= ~IPS_EXPECTED;
		nfct_set_attr_u32(ct, ATTR_STATUS, status);
	}

	nfct_setobjopt(ct, NFCT_SOPT_SETUP_REPLY);

	/*
	 * TCP flags to overpass window tracking for recovered connections
	 */
	if (nfct_attr_is_set(ct, ATTR_TCP_STATE)) {
		uint8_t flags = IP_CT_TCP_FLAG_BE_LIBERAL |
				IP_CT_TCP_FLAG_SACK_PERM;

		if (nfct_get_attr_u8(ct, ATTR_TCP_STATE) >=
						TCP_CONNTRACK_TIME_WAIT) {
			flags |= IP_CT_TCP_FLAG_CLOSE_INIT;
		}
		nfct_set_attr_u8(ct, ATTR_TCP_FLAGS_ORIG, flags);
		nfct_set_attr_u8(ct, ATTR_TCP_MASK_ORIG, flags);
		nfct_set_attr_u8(ct, ATTR_TCP_FLAGS_REPL, flags);
		nfct_set_attr_u8(ct, ATTR_TCP_MASK_REPL, flags);
	}

	int retry = 1;
retry:
	ret = nfct_query(h, NFCT_Q_CREATE, ct);

    if( errno == EEXIST && retry ) {
		//Put failed, destroy ct
		ret = nl_destroy_conntrack(h, ct);
        if (ret == 0 || (ret == -1 && errno == ENOENT)) {
            if (retry) {
                retry = 0;
                goto retry;
            }
        }

	}
	nfct_destroy(ct); 

	return ret;
}

int local_put_perflow(int hashkey, PerflowKey *key, char *state)
{
	if (NULL == key || NULL == state)
    { return -1; }
	int ret;
	h = nfct_open(CONNTRACK, 0);
    if (NULL == h)
    { 
        fprintf(stderr, "nfct_open FAILED: %s\n", strerror(errno));
        return -1;  
    }

	// ct object to insert
    char __obj[nfct_maxsize()];
 	struct nf_conntrack *ct = (struct nf_conntrack *)(void*) __obj;
	// ct object encapsulation
	size_t nethdr_len = 0;
	struct nethdr *net;

	net = (struct nethdr *) sdmbn_base64_decode(state);
	HDR_NETWORK2HOST(net);
	nethdr_len = net->len;
	memset(ct, 0, sizeof(__obj));

	ret = parse_payload(ct, net,  nethdr_len);
	if(ret != 0) {
		fprintf(stderr, "Error parsing nethdr structure. Err:%d\n", ret);
		nfct_close(h);
		return -1;
	}
	ret = nl_create_conntrack(ct);
	if(ret != 0) {
		fprintf(stderr, "Error inserting ct. Err:%d\n", ret);					
	}

	nfct_close(h);
	return 1;	
}


int del_count;
struct nfct_handle *handle_del;
// Collects the data from conntrack queries
int conntrack_del_callback(enum nf_conntrack_msg_type type, 
        struct nf_conntrack *ct, void *data)
{
	struct get_perflow_args args;
	args = *((struct get_perflow_args *)data);
    PerflowKey *key = args.key;
    int id = args.id;

	if (!(key->wildcards & WILDCARD_TP_SRC))
	{ 
		if (!nfct_attr_is_set(ct, ATTR_PORT_SRC))
        return NFCT_CB_CONTINUE;
		uint16_t s_port = nfct_get_attr_u16(ct, ATTR_PORT_SRC);
		if ( s_port != key->tp_src)
		return NFCT_CB_CONTINUE;
	}
	if (!(key->wildcards & WILDCARD_TP_DST))
	{ 
		if (!nfct_attr_is_set(ct, ATTR_PORT_DST))
        return NFCT_CB_CONTINUE;
		uint16_t d_port = nfct_get_attr_u16(ct, ATTR_PORT_DST);
		if ( d_port != key->tp_dst)
		return NFCT_CB_CONTINUE;
	}
	
	//deletes conntrack state
	nl_destroy_conntrack(handle_del, ct);
    // Increment count
	//printf("del_count = %d\n",del_count);
    del_count++;
    return NFCT_CB_CONTINUE;
}

int local_del_perflow(PerflowKey *key, int id) {
	if (NULL == key)
    { return -1; }
	
    del_count = 0;
	// Open a ctnetlink handler
    handle_del = nfct_open(CONNTRACK, 0);
    if (NULL == handle_del)
    { 
        fprintf(stderr, "nfct_open FAILED: %s\n", strerror(errno));
        return -1;  
    }
    // Construct a filter 
    struct nfct_filter *filter = nfct_filter_create();
    if (NULL == filter)
    {
        fprintf(stderr, "nfct_filter_create FAILED: %s\n", strerror(errno));
        return -1;
    }
	
	// Determine dl_type to query
    u_int8_t family = -1;
    if (!(key->wildcards & WILDCARD_DL_TYPE))
    {
        switch(key->dl_type)
        {
        case ETHERTYPE_IP:
            family = AF_INET;
            break;  
        case ETHERTYPE_IPV6:
            family = AF_INET6;
            break;
		default:
			printf("protocol family not supported. Get failed\n");
			return -1;
        }
    }	
	// Add nw_proto to filter
    if (!(key->wildcards & WILDCARD_NW_PROTO))
		nfct_filter_add_attr_u32(filter, NFCT_FILTER_L4PROTO, key->nw_proto);
	
	if (family==AF_INET)
	{
		//IPv4
		// Add nw_src to filter
		if (!(key->wildcards & WILDCARD_NW_SRC))
		{
		    struct nfct_filter_ipv4 filterIP;
		    filterIP.addr = ntohl(key->nw_src);
		    filterIP.mask = 0xFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIP.mask = filterIP.mask << (32 - key->nw_src_mask);
			}
			nfct_filter_add_attr(filter, NFCT_FILTER_SRC_IPV4, &filterIP);
		}

		// Add nw_dst to filter
		if (!(key->wildcards & WILDCARD_NW_DST))
		{
		    struct nfct_filter_ipv4 filterIP;
		    filterIP.addr = ntohl(key->nw_dst);
		    filterIP.mask = 0xFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIP.mask = filterIP.mask << (32 - key->nw_src_mask);
			}
		    nfct_filter_add_attr(filter, NFCT_FILTER_DST_IPV4, &filterIP);
		}
	}
	else if (family==AF_INET6)
	{
		//TODO: IPv6 - not supported currently
		// Add nw_src to filter
		/*if (!(key->wildcards & WILDCARD_NW_SRC))
		{
		    struct nfct_filter_ipv6 filterIPv6;
		    filterIPv6.addr = key->nw_src;
		    filterIPv6.mask = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIPv6.mask = filterIPv6.mask << (128 - key->nw_src_mask);
			}		    
			nfct_filter_add_attr(filter, NFCT_FILTER_SRC_IPV6, &filterIPv6);
		}

		// Add nw_dst to filter
		if (!(key->wildcards & WILDCARD_NW_DST))
		{
		    struct nfct_filter_ipv6 filterIPv6;
		    filterIPv6.addr = key->nw_dst;
			filterIPv6.mask = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
			if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
            { 
				filterIPv6.mask = filterIPv6.mask << (128 - key->nw_src_mask);
			}
		    nfct_filter_add_attr(filter, NFCT_FILTER_DST_IPV6, &filterIPv6);
		}*/
	}
	
	if (nfct_filter_attach(nfct_fd(handle_del), filter) < 0)
    {
        fprintf(stderr, "nfct_filter_attach FAILED: %s\n", strerror(errno));
        return -1;
    }

	// Register call back
	struct get_perflow_args args;
    args.key = key;
    args.id = id;
    nfct_callback_register(handle_del, NFCT_T_ALL, conntrack_del_callback, &args);
	// Issue query
    if (nfct_query(handle_del, NFCT_Q_DUMP, &family) < 0)
    {
        fprintf(stderr,"nfct_query FAILED: %s\n", strerror(errno));
        return -1;
    }

    // Clean-up
    nfct_filter_destroy(filter);
    nfct_close(handle_del);

	return del_count;
}


///// SDMBN Local Config State Handlers //////////////////////////////////////
int local_get_config(int id)
{
	FILE *pf;
	char* state;
	char data[BUFF_SIZE];
	int size = 0;
	char* cmd = "iptables-save|gzip -c|base64 -w 0";    
	
	state = (char*) malloc( sizeof(char)*BUFF_SIZE + sizeof(char));

	/* Setup our pipe for reading and execute our command. */
    pf = popen(cmd, "r"); 

	if(!pf){
      fprintf(stderr, "Could not open pipe for input.\n");
      free(state);
	  return NULL;
    }

    while(fgets(data, BUFF_SIZE, pf)) {
		
		if(size>0){
			state = realloc(state, sizeof(char)*size + sizeof(char)*BUFF_SIZE + sizeof(char));
		}

		strcpy(&state[size], data);
		size += strlen(data);
	}

	if (pclose(pf) != 0)
        fprintf(stderr," Error: Failed to close command stream \n");

	/* check for error in pipe */
	if(strlen(state) <= MIN_SIZE) {
		free(state);
		return -1;
	}
    // Send configuration
	int result = sdmbn_send_config(id, state, 0);
    // Clean-up
    free(state);

	return 1;
}


int local_put_config(int hashkey, char *state)
{
    FILE *pf;
	int rc;
	char* cmd = "base64 -d|gzip -d|iptables-restore --noflush";

	/* Setup our pipe for reading and execute our command. */
	pf = popen(cmd, "w");

	if(!pf){
      fprintf(stderr, "Could not open pipe for output.\n");
      return 1;
	}

	fputs(state, pf);
	fputc('\n', pf);

	if ( ( rc = pclose(pf) ) != 0) {
        fprintf(stderr," Error: Failed to close command stream \n");
		return rc;    
	}

	return 1;
}


