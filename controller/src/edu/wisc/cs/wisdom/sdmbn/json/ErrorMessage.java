package edu.wisc.cs.wisdom.sdmbn.json;

public class ErrorMessage extends Message
{
	public int hashkey;
	public String cause;
	
	public ErrorMessage()
	{ super(Message.RESPONSE_ERROR); }
	
	public ErrorMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_ERROR))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.hashkey = msg.hashkey;
		this.cause = msg.cause;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",hashkey="+hashkey+",cause=\""+cause+"}"; }
}
