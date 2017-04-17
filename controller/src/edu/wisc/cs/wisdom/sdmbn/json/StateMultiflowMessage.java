package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class StateMultiflowMessage extends StateMessage 
{
	public int seq;
	public PerflowKey key;
	
	public StateMultiflowMessage()
	{ super(Message.RESPONSE_STATE_MULTIFLOW); }
	
	public StateMultiflowMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_STATE_MULTIFLOW))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.seq = msg.seq;
		this.key = msg.key;
	}
	
	public String toString()
	{
		return "{id="+id+",type=\""+type+"\",seq="+seq+",key="+key
				+",hashkey="+hashkey+",state=\""+state+"\"}";
	}
}
