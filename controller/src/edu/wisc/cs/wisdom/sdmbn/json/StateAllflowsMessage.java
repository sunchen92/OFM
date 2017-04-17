package edu.wisc.cs.wisdom.sdmbn.json;

public class StateAllflowsMessage extends StateMessage 
{
	public StateAllflowsMessage()
	{ super(Message.RESPONSE_STATE_ALLFLOWS); }
	
    public StateAllflowsMessage(AllFieldsMessage msg) throws MessageException
    {
    	super(msg);
		if (!msg.type.equals(Message.RESPONSE_STATE_ALLFLOWS))
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
