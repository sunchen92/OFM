package edu.wisc.cs.wisdom.sdmbn.json;

public abstract class PutAckMessage extends Message 
{
	public int hashkey;

	public PutAckMessage(String type)
	{ super(type); }
	
	public PutAckMessage(AllFieldsMessage msg)
	{
		super(msg); 
		this.hashkey = msg.hashkey;
	}
	
	public abstract String toString();
}
