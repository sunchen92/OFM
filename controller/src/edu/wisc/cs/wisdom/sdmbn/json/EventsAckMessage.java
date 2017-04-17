package edu.wisc.cs.wisdom.sdmbn.json;

public class EventsAckMessage extends Message 
{
	public EventsAckMessage()
	{ super(Message.RESPONSE_EVENTS_ACK); }
	
	public EventsAckMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_EVENTS_ACK))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\"}"; }
}
