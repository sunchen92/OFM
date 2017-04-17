package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class DelPerflowMessage extends Message
{
	public PerflowKey key;
	
	public DelPerflowMessage()
	{ super(Message.COMMAND_DELETE_PERFLOW); }
	
	public DelPerflowMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_DELETE_PERFLOW))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.key = msg.key;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",key="+key+"}"; }
}
