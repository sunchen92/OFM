package edu.wisc.cs.wisdom.sdmbn.json;

public class ReprocessMessage extends Message
{
	public int hashkey;
	public String header;
	public String packet;
	
	public ReprocessMessage()
	{ super(Message.EVENT_REPROCESS); }

    public ReprocessMessage(AllFieldsMessage msg) throws MessageException
    {
        super(msg);
		if (!msg.type.equals(Message.EVENT_REPROCESS))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
        this.hashkey = msg.hashkey;
        this.header = msg.header;
        this.packet = msg.packet;
    }
	
	public String toString()
	{
		return "{id="+id+",type=\""+type+"\",hashkey="+hashkey
				+",header=\""+header+"\",packet=\""+packet+"\"}";
	}
}
