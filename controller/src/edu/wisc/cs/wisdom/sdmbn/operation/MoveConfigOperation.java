package edu.wisc.cs.wisdom.sdmbn.operation;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.core.StateChunk;
import edu.wisc.cs.wisdom.sdmbn.json.GetConfigAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetConfigMessage;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;
import edu.wisc.cs.wisdom.sdmbn.json.PutConfigAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutConfigMessage;
import edu.wisc.cs.wisdom.sdmbn.json.ReprocessMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateConfigMessage;

public class MoveConfigOperation extends Operation 
{
	private Middlebox src;
	private Middlebox dst;
	private PerflowKey key;
	
	private boolean getAcked;
	private volatile int putAcks;
	private volatile int getCount;
	private boolean isFirstStateRcvd;
	
	public MoveConfigOperation(IOperationManager manager, Middlebox src, 
			Middlebox dst, PerflowKey key)
	{
		super(manager);
		this.src = src;
		this.dst = dst;
		this.key = key;
		this.getAcked = false;
		this.putAcks = 0;
		this.getCount = 0;
		 this.isFirstStateRcvd = false;
	}
		
	@Override
	public int execute()
	{
		GetConfigMessage getConfig = new GetConfigMessage();
		getConfig.id = this.getId();
		getConfig.key = this.key;
		
		try
		{ src.getStateChannel().sendMessage(getConfig); }
		catch(MessageException e)
		{ return -1; }
		
		return this.getId();
	}

	@Override
	public void receiveStateConfig(StateConfigMessage msg) 
	{
		if(this.isFirstStateRcvd == false) {
            log.debug("Copy Start ({})", this.getId());
            this.isFirstStateRcvd = true;
        }

		// Add state chunk to list of state associated with this operation
		obtainStateChunk(msg.hashkey);
		
		PutConfigMessage putConfig = new PutConfigMessage();
		putConfig.id = msg.id;
		putConfig.seq = msg.seq;
		putConfig.key = msg.key;
		putConfig.hashkey = msg.hashkey;
		putConfig.state = msg.state;
		
		try
		{ dst.getStateChannel().sendMessage(putConfig); }
		catch(MessageException e)
		{ log.info("[SDMBN] - "+e); }
	}

	@Override
	public void receivePutConfigAck(PutConfigAckMessage msg) 
	{
		StateChunk chunk = obtainStateChunk(msg.hashkey);
		
		this.putAcks++;
		
		// Indicate state chunk has been acked
		chunk.receivedPutAck();
		
		checkIfFinished();
	}

	@Override
	public void receiveReprocess(ReprocessMessage msg, Middlebox from) 
	{	
		log.warn("Events received for Copy operation");
        assert false; // Should not receive any events for moveConfig operation
	}

	@Override
	public void receiveGetConfigAck(GetConfigAckMessage msg) 
	{
		this.getAcked = true;
		this.getCount = msg.count;
		checkIfFinished();
	}
	
	private void checkIfFinished()
	{
		if (this.getAcked && (this.putAcks == this.getCount))
		{
			log.debug("End moveConfig ({})", this.getId());
			this.finish();
		}
	}

}
