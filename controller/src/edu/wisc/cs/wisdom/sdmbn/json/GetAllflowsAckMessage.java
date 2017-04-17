package edu.wisc.cs.wisdom.sdmbn.json;

public class GetAllflowsAckMessage extends Message 
{
	public int count;
	
	public GetAllflowsAckMessage()
	{ super(Message.RESPONSE_GET_ALLFLOWS_ACK); }
	
	public GetAllflowsAckMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_GET_ALLFLOWS_ACK))
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
