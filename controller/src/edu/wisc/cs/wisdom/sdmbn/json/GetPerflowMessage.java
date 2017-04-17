package edu.wisc.cs.wisdom.sdmbn.json;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class GetPerflowMessage extends Message
{
	public PerflowKey key;
	public int raiseEvents;
	
	public GetPerflowMessage()
	{ this(0, null, true); }
	
	public GetPerflowMessage(int id, PerflowKey key, boolean raiseEvents)
	{
		super(Message.COMMAND_GET_PERFLOW, id);
		this.key = key;
		this.raiseEvents = (raiseEvents ? 1 : 0);
	}
	
	public GetPerflowMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.COMMAND_GET_PERFLOW))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.key = msg.key;
		this.raiseEvents = msg.raiseEvents;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",key="+key+",raiseEvents="+raiseEvents+"}"; }
}
