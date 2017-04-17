package edu.wisc.cs.wisdom.sdmbn.json;

public class DelMultiflowAckMessage extends Message
{
	public int count;
	
	public DelMultiflowAckMessage()
	{ super(Message.RESPONSE_DELETE_MULTIFLOW_ACK); }
	
	public DelMultiflowAckMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_DELETE_MULTIFLOW_ACK))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.count = msg.count;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",count="+count+"}"; }
}
