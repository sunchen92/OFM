package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class AllFieldsMessage extends Message
{
	public int count;
	public PerflowKey key;
	public int deltype;
	public String host;
	public int pid;
	public String action;
	public int hashkey;
	public String cause;
	public int raiseEvents;
	public String state;
	public int seq;
	public String header;
	public String packet;
	
	public Message cast() throws MessageException
	{
		if (this.type.equals(Message.COMMAND_DELETE_MULTIFLOW))
		{ return new DelMultiflowMessage(this); }
		if (this.type.equals(Message.COMMAND_DELETE_PERFLOW))
		{ return new DelPerflowMessage(this); }
		if (this.type.equals(Message.COMMAND_DISABLE_EVENTS))
		{ return new DisableEventsMessage(this); }
		if (this.type.equals(Message.COMMAND_ENABLE_EVENTS))
		{ return new EnableEventsMessage(this); }
		if (this.type.equals(Message.COMMAND_GET_ALLFLOWS))
		{ return new GetAllflowsMessage(this); }
		if (this.type.equals(Message.COMMAND_GET_CONFIG))
		{ return new GetConfigMessage(this); }
		if (this.type.equals(Message.COMMAND_GET_MULTIFLOW))
		{ return new GetMultiflowMessage(this); }
		if (this.type.equals(Message.COMMAND_GET_PERFLOW))
		{ return new GetPerflowMessage(this); }
		if (this.type.equals(Message.COMMAND_PUT_ALLFLOWS))
		{ return new PutAllflowsMessage(this); }
		if (this.type.equals(Message.COMMAND_PUT_CONFIG))
		{ return new PutConfigMessage(this); }
		if (this.type.equals(Message.COMMAND_PUT_MULTIFLOW))
		{ return new PutMultiflowMessage(this); }
		if (this.type.equals(Message.COMMAND_PUT_PERFLOW))
		{ return new PutPerflowMessage(this); }
		if (this.type.equals(Message.EVENT_REPROCESS))
		{ return new ReprocessMessage(this); }
		if (this.type.equals(Message.RESPONSE_DELETE_MULTIFLOW_ACK))
		{ return new DelMultiflowAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_DELETE_PERFLOW_ACK))
		{ return new DelPerflowAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_ERROR))
		{ return new ErrorMessage(this); }
		if (this.type.equals(Message.RESPONSE_EVENTS_ACK))
		{ return new EventsAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_GET_ALLFLOWS_ACK))
		{ return new GetAllflowsAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_GET_CONFIG_ACK))
		{ return new GetConfigAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_GET_MULTIFLOW_ACK))
		{ return new GetMultiflowAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_GET_PERFLOW_ACK))
		{ return new GetPerflowAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_PUT_ALLFLOWS_ACK))
		{ return new PutAllflowsAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_PUT_CONFIG_ACK))
		{ return new PutConfigAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_PUT_MULTIFLOW_ACK))
		{ return new PutMultiflowAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_PUT_PERFLOW_ACK))
		{ return new PutPerflowAckMessage(this); }
		if (this.type.equals(Message.RESPONSE_STATE_ALLFLOWS))
		{ return new StateAllflowsMessage(this); }
		if (this.type.equals(Message.RESPONSE_STATE_CONFIG))
		{ return new StateConfigMessage(this); }
		if (this.type.equals(Message.RESPONSE_STATE_MULTIFLOW))
		{ return new StateMultiflowMessage(this); }
		if (this.type.equals(Message.RESPONSE_STATE_PERFLOW))
		{ return new StatePerflowMessage(this); }
		if (this.type.equals(Message.RESPONSE_SYN))
		{ return new SynMessage(this); }
		
		throw new MessageException(String.format("Unknown type %s", this.type), 
				this);
	}
}
