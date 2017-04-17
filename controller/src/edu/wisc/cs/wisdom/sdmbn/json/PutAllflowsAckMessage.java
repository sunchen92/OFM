package edu.wisc.cs.wisdom.sdmbn.json;

public class PutAllflowsAckMessage extends PutAckMessage
{	
	public PutAllflowsAckMessage()
	{ super(Message.RESPONSE_PUT_ALLFLOWS_ACK); }
	
	public PutAllflowsAckMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_PUT_ALLFLOWS_ACK))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",hashkey="+hashkey+"}"; }
}
