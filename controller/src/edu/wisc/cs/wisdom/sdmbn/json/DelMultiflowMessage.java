package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class DelMultiflowMessage extends Message
{
	public PerflowKey key;
	public int deltype;
	
	public DelMultiflowMessage()
	{ super(Message.COMMAND_DELETE_MULTIFLOW); }
	
	public DelMultiflowMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_DELETE_MULTIFLOW))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.key = msg.key;
		this.deltype = msg.deltype;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",key="+key+"\",deltype="+deltype+"}"; }
}
