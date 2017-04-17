package edu.wisc.cs.wisdom.sdmbn.json;

public class PutPerflowAckMessage extends PutAckMessage
{
	public PutPerflowAckMessage()
	{ super(Message.RESPONSE_PUT_PERFLOW_ACK); }
	
	public PutPerflowAckMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_PUT_PERFLOW_ACK))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",hashkey="+hashkey+"}"; }
}
