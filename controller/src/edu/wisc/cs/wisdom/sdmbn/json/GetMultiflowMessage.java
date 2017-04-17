package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class GetMultiflowMessage extends Message
{
	public PerflowKey key;
		
	public GetMultiflowMessage()
	{ this(0, null); }
	
	public GetMultiflowMessage(int id, PerflowKey key)
	{
		super(Message.COMMAND_GET_MULTIFLOW, id);
		this.key = key;
	}
	
	public GetMultiflowMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_GET_MULTIFLOW))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.key = msg.key;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",key="+key+"}"; }
}
