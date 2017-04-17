package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class PutPerflowMessage extends PutMessage
{
	public int seq;
	public PerflowKey key;
	
	public PutPerflowMessage()
	{ super(Message.COMMAND_PUT_PERFLOW); }
	
	public PutPerflowMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_PUT_PERFLOW))
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
