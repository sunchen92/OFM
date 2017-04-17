package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class DisableEventsMessage extends Message
{
	public PerflowKey key;
	
	public DisableEventsMessage()
	{ super(Message.COMMAND_DISABLE_EVENTS); }
	
	public DisableEventsMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_DISABLE_EVENTS))
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
