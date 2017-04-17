package edu.wisc.cs.wisdom.sdmbn.json;

public class PutAllflowsMessage extends PutMessage
{	
	public PutAllflowsMessage()
	{ super(Message.COMMAND_PUT_ALLFLOWS); }
	
	public PutAllflowsMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_PUT_ALLFLOWS))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
	}
	
	public String toString()
	{
		return "{id="+id+",type=\""+type+"\",hashkey="+hashkey+",state=\""+state+"\"}";
	}
}
