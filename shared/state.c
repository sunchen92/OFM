#include "SDMBN.h"
#include "SDMBNCore.h"
#include "SDMBNConn.h"
#include "SDMBNDebug.h"
#include "SDMBNJson.h"
#include "SDMBNConfig.h"
#include "event.h"
#include "state.h"
#include <json/json.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

// Thread for handling state messages
static pthread_t state_thread;

// Connection for state messages
int sdmbn_conn_state = -1;

static void send_delete_perflow_ack(int id, int count)
{
    json_object *ack = json_compose_delete_perflow_ack(id, count);
    json_send(ack, sdmbn_conn_state);
    json_object_put(ack);
}

static void send_delete_multiflow_ack(int id, int count)
{
    json_object *ack = json_compose_delete_multiflow_ack(id, count);
    json_send(ack, sdmbn_conn_state);
    json_object_put(ack);
}

static void send_get_perflow_ack(int id, int count)
{
    json_object *ack = json_compose_get_perflow_ack(id, count);
    json_send(ack, sdmbn_conn_state);
    json_object_put(ack);
}

static void send_put_perflow_ack(int id, int hashkey)
{
    json_object *ack = json_compose_put_perflow_ack(id, hashkey);
    json_send(ack, sdmbn_conn_event);
    json_object_put(ack);
}

static void send_get_multiflow_ack(int id, int count)
{
    json_object *ack = json_compose_get_multiflow_ack(id, count);
    json_send(ack, sdmbn_conn_state);
    json_object_put(ack);
}

static void send_put_multiflow_ack(int id, int hashkey)
{
    json_object *ack = json_compose_put_multiflow_ack(id, hashkey);
    json_send(ack, sdmbn_conn_event);
    json_object_put(ack);
}

static void send_get_allflows_ack(int id, int count)
{
    json_object *ack = json_compose_get_allflows_ack(id, count);
    json_send(ack, sdmbn_conn_state);
    json_object_put(ack);
}

static void send_put_allflows_ack(int id, int hashkey)
{
    json_object *ack = json_compose_put_allflows_ack(id, hashkey);
    json_send(ack, sdmbn_conn_event);
    json_object_put(ack);
}

static void send_get_config_ack(int id, int count)
{
    json_object *ack = json_compose_get_config_ack(id, count);
    json_send(ack, sdmbn_conn_state);
    json_object_put(ack);
}

static void send_put_config_ack(int id, int hashkey)
{
    json_object *ack = json_compose_put_config_ack(id, hashkey);
    json_send(ack, sdmbn_conn_event);
    json_object_put(ack);
}

static void send_error(int id, int hashkey, char *cause)
{
    json_object *error = json_compose_error(id, hashkey, cause);
    json_send(error, sdmbn_conn_state);
    json_object_put(error);
}

static int handle_get_perflow(json_object *msg, int id)
{
    // Check if command is supported 
    if (NULL == sdmbn_locals->get_perflow)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1; 
    }

    // Parse key
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    {
        ERROR_PRINT("Get perflow has no key");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    PerflowKey key;
    json_parse_perflow_key(key_field, &key);

    // Get raiseEvents flag
	json_object *raiseEvents_field = json_object_object_get(msg, FIELD_RAISEEVENTS);
	if (NULL == raiseEvents_field)
	{
		ERROR_PRINT("JSON from controller has no raiseEvents field");
		send_error(id, -1, ERROR_MALFORMED);
		return -1;
	}
	int raiseEvents = json_object_get_int(raiseEvents_field);

    // Wait for get lock to release
    INFO_PRINT("Wait for get lock to release");
    pthread_mutex_lock(&sdmbn_lock_get);
    INFO_PRINT("Get lock released");

#ifdef SDMBN_STATS
    // Update statistics
    struct timeval start_get, end_get;
    gettimeofday(&start_get, NULL);
#endif 

    // Set up extensions
    SDMBNExt *exts = NULL;
    if (sdmbn_config.max_get_flows > 0)
    {
        SDMBNExt maxChunks;
        maxChunks.id = SDMBN_EXT_MAX_CHUNKS; 
        maxChunks.data = &(sdmbn_config.max_get_flows);
        maxChunks.next = NULL;
        exts = &maxChunks;
    }

    int count = sdmbn_locals->get_perflow(&key, id, raiseEvents, exts);
    if (count < 0)
    {
        ERROR_PRINT("Local get perflow function failed");
        send_error(id, -1, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_get_perflow_ack(id, count);

    // Release get lock
    pthread_mutex_unlock(&sdmbn_lock_get);

#ifdef SDMBN_STATS
    // Update statistics
    gettimeofday(&end_get, NULL);
    long sec = end_get.tv_sec - start_get.tv_sec;
    long usec = end_get.tv_usec - start_get.tv_usec;
    long total = (sec * 1000 * 1000) + usec;
    sdmbn_stats.gets_per_time += total;
    sdmbn_stats.gets_per_count++;
    STATS_PRINT("STATS: gets_per_count=%d, gets_per_time=%ldus", 
            sdmbn_stats.gets_per_count, sdmbn_stats.gets_per_time);
    STATS_PRINT("STATS: num_states_moved = %d", count);
    STATS_PRINT("GETPER,%ld.%ld,%ld.%ld,",start_get.tv_sec,start_get.tv_usec,
            end_get.tv_sec,end_get.tv_usec);
#endif

    return 1;
}

int sdmbn_send_perflow(int id, PerflowKey *key, char *state, int hashkey, 
        int seq)
{
    if (NULL == key || NULL == state || seq < 0)
    { return -1; }

    // Send perflow support state
    json_object *msg = json_compose_perflow_state(id, key, state, hashkey, 
            seq);
    int result = json_send(msg, sdmbn_conn_state);

    // Free JSON object
    json_object_put(msg);

    return result;
}

static int handle_put_perflow(json_object *msg, int id)
{
    // Check if command is supported 
    if (NULL == sdmbn_locals->put_perflow)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1; 
    }

#ifdef SDMBN_STATS
    // Update statistics
    struct timeval start_put, end_put;
    gettimeofday(&start_put, NULL);
#endif

    // Parse hashkey
    json_object *hashkey_field = json_object_object_get(msg, FIELD_HASHKEY);
    if (NULL == hashkey_field)
    {
        ERROR_PRINT("Put perflow has no hashkey");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    int hashkey = json_object_get_int(hashkey_field);

    // Parse key
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    {
        ERROR_PRINT("Put perflow has no key");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    PerflowKey key;
    json_parse_perflow_key(key_field, &key);

    // Parse state
    json_object *state_field = json_object_object_get(msg, FIELD_STATE);
    if (NULL == state_field)
    {
        ERROR_PRINT("Put perflow has no state");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    char *state = (char *)json_object_get_string(state_field);

    // Call MB-specific function
    int result = sdmbn_locals->put_perflow(hashkey, &key, state);
    if (result < 0)
    {
        ERROR_PRINT("Local put perflow function failed");
        send_error(id, hashkey, ERROR_INTERNAL);
        return -1; 
    }

    // Increment number of active flows
    sdmbn_stats.flows_active++;

    // Send ACK
    send_put_perflow_ack(id, hashkey);

#ifdef SDMBN_STATS
    // Update statistics
    gettimeofday(&end_put, NULL);
    long sec = end_put.tv_sec - start_put.tv_sec;
    long usec = end_put.tv_usec - start_put.tv_usec;
    long total = (sec * 1000 * 1000) + usec;
    sdmbn_stats.puts_per_time += total;
    sdmbn_stats.puts_per_count++;
    STATS_PRINT("STATS: puts_per_count=%d, puts_per_time=%ldus", 
            sdmbn_stats.puts_per_count, sdmbn_stats.puts_per_time);
    STATS_PRINT("PUTPER,%ld.%ld,%ld.%ld", start_put.tv_sec,
            start_put.tv_usec, end_put.tv_sec, end_put.tv_usec);
#endif

    return result;
}

static int handle_get_multiflow(json_object *msg, int id)
{
    // Check if command is supported 
    if (NULL == sdmbn_locals->get_multiflow)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1; 
    }

    // Parse key
    PerflowKey key;
    PerflowKey *keyPtr = NULL;
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    { keyPtr = NULL; }
    else
    {
        json_parse_perflow_key(key_field, &key);
        keyPtr = &key;
    }
    
#ifdef SDMBN_STATS
	// Update statistics
    struct timeval start_get, end_get;
    gettimeofday(&start_get, NULL);
#endif

    int count = sdmbn_locals->get_multiflow(keyPtr, id);
    if (count < 0)
    {
        ERROR_PRINT("Local get multiflow function failed");
        send_error(id, -1, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_get_multiflow_ack(id, count);

#ifdef SDMBN_STATS
    // Update statistics
    gettimeofday(&end_get, NULL);
    long sec = end_get.tv_sec - start_get.tv_sec;
    long usec = end_get.tv_usec - start_get.tv_usec;
    long total = (sec * 1000 * 1000) + usec;
    sdmbn_stats.gets_mul_time += total;
    sdmbn_stats.gets_mul_count++;
    STATS_PRINT("STATS: gets_mul_count=%d, gets_mul_time=%ldus",
	        sdmbn_stats.gets_mul_count, sdmbn_stats.gets_mul_time);
    STATS_PRINT("STATS: num_multiflow_states_moved = %d", count);
    STATS_PRINT("GETMUL,%ld.%ld,%ld.%ld",start_get.tv_sec,start_get.tv_usec,
            end_get.tv_sec,end_get.tv_usec);
#endif

    return 1;
}

int sdmbn_send_multiflow(int id, PerflowKey *key, char *state, int hashkey, 
        int seq)
{
    if (NULL == key || NULL == state || seq < 0)
    { return -1; }

    // Send multiflow support state
    json_object *msg = json_compose_multiflow_state(id, key, state, hashkey, 
            seq);
    int result = json_send(msg, sdmbn_conn_state);

    // Free JSON object
    json_object_put(msg);

    return result;
}

static int handle_put_multiflow(json_object *msg, int id)
{
    // Check if command is supported 
    if (NULL == sdmbn_locals->put_multiflow)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1; 
    }

#ifdef SDMBN_STATS
    // Update statistics
    struct timeval start_put, end_put;
    gettimeofday(&start_put, NULL);
#endif

    // Parse hashkey
    json_object *hashkey_field = json_object_object_get(msg, FIELD_HASHKEY);
    if (NULL == hashkey_field)
    {
        ERROR_PRINT("Put multiflow has no hashkey");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    int hashkey = json_object_get_int(hashkey_field);

    // Parse key
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    {
        ERROR_PRINT("Put multiflow has no key");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    PerflowKey key;
    json_parse_perflow_key(key_field, &key);

    // Parse state
    json_object *state_field = json_object_object_get(msg, FIELD_STATE);
    if (NULL == state_field)
    {
        ERROR_PRINT("Put multiflow has no state");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    char *state = (char *)json_object_get_string(state_field);

    // Call MB-specific function
    int result = sdmbn_locals->put_multiflow(hashkey, &key, state);
    if (result < 0)
    {
        ERROR_PRINT("Local put multiflow function failed");
        send_error(id, hashkey, ERROR_INTERNAL);
        return -1; 
    }

    // Send ACK
    send_put_multiflow_ack(id, hashkey);

#ifdef SDMBN_STATS
    // Update statistics
    gettimeofday(&end_put, NULL);
    long sec = end_put.tv_sec - start_put.tv_sec;
    long usec = end_put.tv_usec - start_put.tv_usec;
    long total = (sec * 1000 * 1000) + usec;
    sdmbn_stats.puts_mul_time += total;
    sdmbn_stats.puts_mul_count++;
    STATS_PRINT("STATS: puts_mul_count=%d, puts_mul_time=%ldus",
            sdmbn_stats.puts_mul_count, sdmbn_stats.puts_mul_time);
    STATS_PRINT("PUTMUL,%ld.%ld,%ld.%ld",start_put.tv_sec,start_put.tv_usec,
            end_put.tv_sec,end_put.tv_usec);
#endif

    return result;
}

static int handle_get_allflows(json_object *msg, int id)
{
    // Check if command is supported
    if (NULL == sdmbn_locals->get_allflows)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1;
    }

    // Call MB-specific function
    int count = sdmbn_locals->get_allflows(id);
    if (count < 0)
    {
        ERROR_PRINT("Local get allflows function failed");
        send_error(id, -1, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_get_allflows_ack(id, count);

    return 1;
}

int sdmbn_send_allflows(int id, char *state, int seq)
{
    if (NULL == state || seq < 0)
    { return -1; }

    // Send allflows support state
    json_object *msg = json_compose_allflows_state(id, state, seq);
    int result = json_send(msg, sdmbn_conn_state);

    // Free JSON object
    json_object_put(msg);

    return result;
}

static int handle_put_allflows(json_object *msg, int id)
{
    // Check if command is supported
    if (NULL == sdmbn_locals->put_allflows)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1;
    }

    // Parse hashkey
    json_object *hashkey_field = json_object_object_get(msg, FIELD_HASHKEY);
    if (NULL == hashkey_field)
    {
        ERROR_PRINT("Put allflows has no hashkey");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    int hashkey = json_object_get_int(hashkey_field);

    // Parse allflows state
    json_object *state_field = json_object_object_get(msg, FIELD_STATE);
    if (NULL == state_field)
    {
        ERROR_PRINT("Put allflows has no state");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    char *state = (char *)json_object_get_string(state_field);

    // Call MB-specific function
    int result = sdmbn_locals->put_allflows(hashkey, state);
    if (result < 0)
    {
        ERROR_PRINT("Local put allfows function failed");
        send_error(id, hashkey, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_put_allflows_ack(id, hashkey);

    return result;
}

static int handle_get_config(json_object *msg, int id)
{
    // Check if command is supported 
    if (NULL == sdmbn_locals->get_config)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1; 
    }

    // Call MB-specific function
    int count = sdmbn_locals->get_config(id);
    if (count < 0)
    {
        ERROR_PRINT("Local get config function failed");
        send_error(id, -1, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_get_config_ack(id, count);

    return 1;
}

int sdmbn_send_config(int id, char *state, int seq)
{
    if (NULL == state || seq < 0)
    { return -1; }

    // Send config state
    json_object *msg = json_compose_config_state(id, state, seq);
    int result = json_send(msg, sdmbn_conn_state);

    // Free JSON object
    json_object_put(msg);

    return result;
}

static int handle_put_config(json_object *msg, int id)
{
    // Check if command is supported 
    if (NULL == sdmbn_locals->put_config)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1; 
    }

    // Parse hashkey
    json_object *hashkey_field = json_object_object_get(msg, FIELD_HASHKEY);
    if (NULL == hashkey_field)
    {
        ERROR_PRINT("Put config has no hashkey");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    int hashkey = json_object_get_int(hashkey_field);

    // Parse config state
    json_object *state_field = json_object_object_get(msg, FIELD_STATE);
    if (NULL == state_field)
    {
        ERROR_PRINT("Put config has no state");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    char *state = (char *)json_object_get_string(state_field);

    // Call MB-specific function
    int result = sdmbn_locals->put_config(hashkey, state);
    if (result < 0)
    {
        ERROR_PRINT("Local put config function failed");
        send_error(id, hashkey, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_put_config_ack(id, hashkey);

    return result;
}

static int handle_delete_perflow(json_object *msg, int id)
{
    // Check if command is supported
    if (NULL == sdmbn_locals->delete_perflow)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1;
    }

    // Parse key
    PerflowKey key;
    PerflowKey *keyPtr = NULL;
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    { keyPtr = NULL; }
    else
    {
        json_parse_perflow_key(key_field, &key);
        keyPtr = &key;
    }

#ifdef SDMBN_STATS
    // Update statistics
    struct timeval start_del, end_del;
    gettimeofday(&start_del, NULL);
#endif

    // Call MB-specific function
    int count = sdmbn_locals->delete_perflow(keyPtr, id);
    if (count < 0)
    {
        ERROR_PRINT("Local delete per flow function failed");
        send_error(id, -1, ERROR_INTERNAL);
        return -1;
    }

    // Decrement number of active flows
    sdmbn_stats.flows_active--;

    // Send ACK
    send_delete_perflow_ack(id, count);

#ifdef SDMBN_STATS
    // Update statistics
    gettimeofday(&end_del, NULL);
    long sec = end_del.tv_sec - start_del.tv_sec;
    long usec = end_del.tv_usec - start_del.tv_usec;
    long total = (sec * 1000 * 1000) + usec;
    STATS_PRINT("STATS: delete_perflow_count=%d, get_time_sum=%ldus", count, total);
    STATS_PRINT("DELPER,%ld.%ld,%ld.%ld",start_del.tv_sec,start_del.tv_usec,
            end_del.tv_sec,end_del.tv_usec);
#endif

    return 1;
}

static int handle_delete_multiflow(json_object *msg, int id) {
    // Check if command is supported 
    if (NULL == sdmbn_locals->delete_multiflow)
    {
        send_error(id, -1, ERROR_UNSUPPORTED);
        return -1;
    }

    // Parse key
    PerflowKey key;
    PerflowKey *keyPtr = NULL;
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    { keyPtr = NULL; }
    else
    {
        json_parse_perflow_key(key_field, &key);
        keyPtr = &key;
    }

    // Parse key
    json_object *deltype_field = json_object_object_get(msg, FIELD_DELTYPE);
    if (NULL == deltype_field)
    {
        ERROR_PRINT("Delete all has no delete_type");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    int deltype = json_object_get_int(deltype_field);

#ifdef SDMBN_STATS
    // Update statistics
    struct timeval start_del, end_del;
    gettimeofday(&start_del, NULL);
#endif

    int count = sdmbn_locals->delete_multiflow(keyPtr, id, deltype);
    if (count < 0)
    {
        ERROR_PRINT("Local delete multi flows function failed");
        send_error(id, -1, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_delete_multiflow_ack(id, count);

#ifdef SDMBN_STATS
    // Update statistics
    gettimeofday(&end_del, NULL);
    long sec = end_del.tv_sec - start_del.tv_sec;
    long usec = end_del.tv_usec - start_del.tv_usec;
    long total = (sec * 1000 * 1000) + usec;
    STATS_PRINT("STATS: delete_multiflow_count=%d, get_time_sum=%ldus", count, total);
    STATS_PRINT("DELMUL,%ld.%ld,%ld.%ld",start_del.tv_sec,start_del.tv_usec,
            end_del.tv_sec,end_del.tv_usec);
#endif

    return 1;
}

static void *state_handler(void *arg)
{
    INFO_PRINT("State handling thread started");

    json_tokener_new();

    while (1)
    {
        // Attempt to read a JSON string
        char* buffer =  conn_read(sdmbn_conn_state);
        if (NULL == buffer)
        {
            ERROR_PRINT("Failed to read from state socket");
            break; 
        }

        // Attempt to parse the JSON string
        struct json_object *msg;
        msg = json_tokener_parse(buffer);
        free(buffer);
        if (NULL == msg)
        {
            ERROR_PRINT("Failed to parse JSON from controller: %s", buffer);
            continue;
        }
        DEBUG_PRINT("Parsed msg");

        // Get message id
        json_object *id_field = json_object_object_get(msg, FIELD_ID);
        if (NULL == id_field)
        {
            ERROR_PRINT("JSON from controller has no id: %s", buffer);
            send_error(-1, -1, ERROR_MALFORMED);
            // Free JSON object
            json_object_put(msg);
            continue;
        }
        int id = json_object_get_int(id_field);

        // Get message type
        json_object *type_field = json_object_object_get(msg, FIELD_TYPE);
        if (NULL == type_field)
        {
            ERROR_PRINT("JSON from controller has no type: %s", buffer);
            send_error(id, -1, ERROR_MALFORMED);
            // Free JSON object
            json_object_put(msg);
            continue;
        }
        char *type = (char *)json_object_get_string(type_field);
                
        DEBUG_PRINT("Message: id=%d, type=%s", id, type);

        // Take appropriate action
        // TODO: Check whether we support the action
        if (0 == strcmp(type, COMMAND_GET_PERFLOW))
        { handle_get_perflow(msg, id); }
        else if (0 == strcmp(type, COMMAND_PUT_PERFLOW))
        { handle_put_perflow(msg, id); }
        else if (0 == strcmp(type, COMMAND_GET_MULTIFLOW))
		{ handle_get_multiflow(msg, id); }
		else if (0 == strcmp(type, COMMAND_PUT_MULTIFLOW))
		{ handle_put_multiflow(msg, id); }
        else if (0 == strcmp(type, COMMAND_GET_ALLFLOWS))
        { handle_get_allflows(msg, id); }
        else if (0 == strcmp(type, COMMAND_PUT_ALLFLOWS))
        { handle_put_allflows(msg, id); }
        else if (0 == strcmp(type, COMMAND_GET_CONFIG))
        { handle_get_config(msg, id); }
        else if (0 == strcmp(type, COMMAND_PUT_CONFIG))
        { handle_put_config(msg, id); }
        else if (0 == strcmp(type, COMMAND_DELETE_PERFLOW))
        { handle_delete_perflow(msg, id); }
        else if (0 == strcmp(type, COMMAND_DELETE_MULTIFLOW))
        { handle_delete_multiflow(msg, id); }
        else if (0 == strcmp(type, COMMAND_ENABLE_EVENTS))
        { handle_enable_events(msg, id); }
        else if (0 == strcmp(type, COMMAND_DISABLE_EVENTS))
        { handle_disable_events(msg, id); }
        else
        { 
            ERROR_PRINT("Unknown type: %s", type);
            send_error(id, -1, ERROR_MALFORMED);
        }

        // Free JSON object
        json_object_put(msg);
    }

    INFO_PRINT("State handling thread finished");
    pthread_exit(NULL);
}

int state_init()
{
    // Open state communication channel
    sdmbn_conn_state = conn_active_open(sdmbn_config.ctrl_ip, 
            sdmbn_config.ctrl_port_state);
    if (sdmbn_conn_state < 0)
    { 
        ERROR_PRINT("Failed to open state communication channel");
        return sdmbn_conn_state; 
    }

    // Create SYN
    json_object *syn = json_compose_syn();

    // Send SYN
    json_send(syn, sdmbn_conn_state);

    // Free JSON object
    json_object_put(syn);

    // Create SDMBN state handling thread
    pthread_create(&state_thread, NULL, state_handler, NULL);
    return 1;
}

int state_cleanup()
{
    // Close state communication channel
    if (conn_close(sdmbn_conn_state) >= 0)
    { sdmbn_conn_state = -1; }
    else
    { ERROR_PRINT("Failed to close state communication channel"); }

    // Destroy state thread
    //pthread_kill(state_thread, SIGKILL);

    return 1;
}
