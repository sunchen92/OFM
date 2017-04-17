package edu.wisc.cs.wisdom.sdmbn.json;

public class Message 
{
	public static final String FIELD_ID = "id";
	public static final String FIELD_TYPE = "type";
	public static final String FIELD_RAISEEVENTS = "raiseEvents";
	public static final String FIELD_HASHKEY = "hashkey";
	public static final String FIELD_STATE = "state";
	public static final String FIELD_SEQ = "seq";
	public static final String FIELD_COUNT = "count";
	public static final String FIELD_CAUSE = "cause";
	public static final String FIELD_PACKET = "packet";
	public static final String FIELD_HOST = "host";
	public static final String FIELD_ACTION = "action";
	
	public static final String COMMAND_GET_PERFLOW = "get-perflow";
	public static final String COMMAND_PUT_PERFLOW = "put-perflow";
	public static final String COMMAND_GET_MULTIFLOW = "get-multiflow";
	public static final String COMMAND_PUT_MULTIFLOW = "put-multiflow";
	public static final String COMMAND_GET_ALLFLOWS = "get-allflows";
	public static final String COMMAND_PUT_ALLFLOWS = "put-allflows";
	public static final String COMMAND_GET_CONFIG = "get-config";
	public static final String COMMAND_PUT_CONFIG = "put-config";
	public static final String COMMAND_DELETE_MULTIFLOW  = "delete-multiflow";
	public static final String COMMAND_DELETE_PERFLOW  = "delete-perflow";
	public static final String COMMAND_ENABLE_EVENTS = "enable-events";
	public static final String COMMAND_DISABLE_EVENTS = "disable-events";
	
	public static final String RESPONSE_GET_PERFLOW_ACK = "get-perflow-ack";
	public static final String RESPONSE_PUT_PERFLOW_ACK = "put-perflow-ack";
	public static final String RESPONSE_GET_MULTIFLOW_ACK = "get-multiflow-ack";
	public static final String RESPONSE_PUT_MULTIFLOW_ACK = "put-multiflow-ack";
	public static final String RESPONSE_GET_ALLFLOWS_ACK = "get-allflows-ack";
	public static final String RESPONSE_PUT_ALLFLOWS_ACK = "put-allflows-ack";
	public static final String RESPONSE_GET_CONFIG_ACK = "get-config-ack";
	public static final String RESPONSE_PUT_CONFIG_ACK = "put-config-ack";
	public static final String RESPONSE_EVENTS_ACK = "events-ack";
	public static final String RESPONSE_SYN = "syn";
	public static final String RESPONSE_ERROR = "error";
	public static final String RESPONSE_STATE_PERFLOW = "state-perflow";
	public static final String RESPONSE_STATE_MULTIFLOW = "state-multiflow";
	public static final String RESPONSE_STATE_ALLFLOWS = "state-allflows";
	public static final String RESPONSE_STATE_CONFIG = "state-config";
	public static final String RESPONSE_DELETE_PERFLOW_ACK = "delete-perflow-ack";
	public static final String RESPONSE_DELETE_MULTIFLOW_ACK = "delete-multiflow-ack";
	
	public static final String EVENT_REPROCESS = "reprocess";
	
	public static final String CONSTANT_ACTION_DROP = "drop";
	public static final String CONSTANT_ACTION_BUFFER = "buffer";
	public static final String CONSTANT_ACTION_PROCESS = "process";
		
	public static final String ERROR_MALFORMED = "malformed";
	public static final String ERROR_UNSUPPORED = "unsupported";
	public static final String ERROR_INTERNAL = "internal-failure";
	
	public String type;
	public int id;
	
	public Message()
	{ this(null,0); }
	
	public Message(AllFieldsMessage msg)
	{ this(msg.type, msg.id); }
	
	public Message(String type)
	{ this(type, 0); }
	
	public Message(String type, int id)
	{
		this.type = type;
		this.id = id;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\"}"; }
}
