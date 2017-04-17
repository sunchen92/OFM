package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class EnableEventsMessage extends Message
{
	public PerflowKey key;
	public String action;
	
	public EnableEventsMessage()
	{ super(Message.COMMAND_ENABLE_EVENTS); }
	
	public EnableEventsMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_ENABLE_EVENTS))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.key = msg.key;
		this.action = msg.action;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",key="+key+"\",action=\""+action+"\"}"; }
}
