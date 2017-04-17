package edu.wisc.cs.wisdom.sdmbn.json;

public class DelPerflowAckMessage extends Message
{
	public int count;
	
	public DelPerflowAckMessage()
	{ super(Message.RESPONSE_DELETE_PERFLOW_ACK); }
	
	public DelPerflowAckMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_DELETE_PERFLOW_ACK))
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
