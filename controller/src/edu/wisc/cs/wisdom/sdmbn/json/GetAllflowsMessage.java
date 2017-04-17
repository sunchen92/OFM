package edu.wisc.cs.wisdom.sdmbn.json;

public class GetAllflowsMessage extends Message
{
	public GetAllflowsMessage()
	{ super(Message.COMMAND_GET_ALLFLOWS); }
	
	public GetAllflowsMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_GET_ALLFLOWS))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"}"; }
}
