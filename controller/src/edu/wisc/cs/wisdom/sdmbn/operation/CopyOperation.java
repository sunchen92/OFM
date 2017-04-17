package edu.wisc.cs.wisdom.sdmbn.operation;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Scope;
import edu.wisc.cs.wisdom.sdmbn.core.StateChunk;
import edu.wisc.cs.wisdom.sdmbn.json.GetAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;
import edu.wisc.cs.wisdom.sdmbn.json.PutAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.ReprocessMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StatePerflowMessage;

public class CopyOperation extends Operation 
{
	// Arguments provided in constructor
    private Middlebox src;
    private Middlebox dst;
    private PerflowKey key;
    private Scope scope;

    // Operation state
    private boolean getPerflowAcked;
    private boolean getMultiflowAcked;
    private boolean getAllflowsAcked;
    private volatile int getPerflowCount;
    private volatile int getMultiflowCount;
    private volatile int getAllflowsCount;
    private volatile int putPerflowAcks;
    private volatile int putMultiflowAcks;
    private volatile int putAllflowsAcks;

    // State processing
    private ExecutorService threadPool;
    
    // Statistics
    private boolean isFirstStateRcvd;
    private long copyStart;

    public CopyOperation(IOperationManager manager, Middlebox src, 
            Middlebox dst, PerflowKey key, Scope scope)
    {
    	// Store arguments
        super(manager);
        this.src = src;
        this.dst = dst;
        this.key = key;
        this.scope = scope;

        // Initialize operation state variables
        this.getPerflowAcked = false;
        this.getMultiflowAcked = false;
        this.getAllflowsAcked = false;
        this.getPerflowCount = 0;
        this.getMultiflowCount = 0;
        this.getAllflowsCount = 0;
        this.putPerflowAcks = 0;
        this.putMultiflowAcks = 0;
        this.putAllflowsAcks = 0;
        
        // Initialize state and processing variables
        this.threadPool = Executors.newCachedThreadPool();

        // Initialize statistics variables
        this.isFirstStateRcvd = false;
        this.copyStart = -1;
    }

    @Override
    public int execute()
    {
		GetPerflowMessage getPerflow = null;
		GetMultiflowMessage getMultiflow = null;
		GetAllflowsMessage getAllflows = null;
		if (Scope.PERFLOW == this.scope || Scope.PF_MF == this.scope 
				|| Scope.PF_AF == this.scope || Scope.PF_MF_AF == this.scope)
		{
			getPerflow = new GetPerflowMessage();
			getPerflow.id = this.getId();
    		getPerflow.key = this.key;
    		getPerflow.raiseEvents = 0;
    		try
    		{ this.src.getStateChannel().sendMessage(getPerflow); }
    		catch(MessageException e)
    		{ return -1; }
		}
		
		if (Scope.MULTIFLOW == this.scope || Scope.PF_MF == this.scope 
				|| Scope.MF_AF == this.scope || Scope.PF_MF_AF == this.scope)
		{
			getMultiflow = new GetMultiflowMessage();
			getMultiflow.id = this.getId();
			getMultiflow.key = this.key;
    		try
    		{ this.src.getStateChannel().sendMessage(getMultiflow); }
    		catch(MessageException e)
    		{ return -1; }
		}
		if (Scope.ALLFLOWS == this.scope || Scope.PF_AF == this.scope 
				|| Scope.MF_AF == this.scope || Scope.PF_MF_AF == this.scope)
		{
			getAllflows = new GetAllflowsMessage();
			getAllflows.id = this.getId();
    		try
    		{ this.src.getStateChannel().sendMessage(getAllflows); }
    		catch(MessageException e)
    		{ return -1; }
		}
		if (null == getPerflow && null == getMultiflow && null == getAllflows)
		{
			log.error("Scope needs to be 'PERFLOW', 'MULTIFLOW', 'ALLFLOWS', 'PF_MF', 'PF_AF', 'MF_AF' or 'PF_MF_AF'");
			return -1;
		}

        return this.getId();
    }

    @Override
    public void receiveStatePerflow(StatePerflowMessage msg) 
    {
        if (this.isFirstStateRcvd == false) 
        {
        	this.copyStart = System.currentTimeMillis();
            log.debug("Copy Start ({})", this.getId());
            this.isFirstStateRcvd = true;
        }

        // Add state chunk to list of state associated with this operation
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        chunk.storeStateMessage(msg, this.dst);
        
        // Process chunk
        this.threadPool.submit(chunk);

        /*PutPerflowMessage putPerflow = new PutPerflowMessage();
        putPerflow.id = msg.id;
        putPerflow.seq = msg.seq;
        putPerflow.key = msg.key;
        putPerflow.hashkey = msg.hashkey;
        putPerflow.state = msg.state;

        try
        { this.dst.getStateChannel().sendMessage(putPerflow); }
        catch(Exception e)
        {
            log.error(e.toString()); 
            this.fail();
        }*/
    }

    @Override
    public void receiveStateMultiflow(StateMultiflowMessage msg) 
    { 
    	if (this.isFirstStateRcvd == false) 
    	{
    		this.copyStart = System.currentTimeMillis();
        	log.debug("Copy Start ({})", this.getId());
        	this.isFirstStateRcvd = true;
    	}
    	
        // Add state chunk to list of state associated with this operation
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        chunk.storeStateMessage(msg, this.dst);
        
        // Process chunk
        this.threadPool.submit(chunk);
        
        /*PutMultiflowMessage putMultiflow = new PutMultiflowMessage();
        putMultiflow.id = msg.id;
        putMultiflow.seq = msg.seq;
        putMultiflow.key = msg.key;
        putMultiflow.hashkey = msg.hashkey;
        putMultiflow.state = msg.state;

        try
        { this.dst.getStateChannel().sendMessage(putMultiflow); }
        catch(Exception e)
        { 
            log.error(e.toString());
            this.fail();
        }*/
    }

    @Override
    public void receiveStateAllflows(StateAllflowsMessage msg) 
    { 
    	if(this.isFirstStateRcvd == false) 
    	{
    		this.copyStart = System.currentTimeMillis();
        	log.debug("Copy Start ({})", this.getId());
        	this.isFirstStateRcvd = true;
    	}
    	
        // Add state chunk to list of state associated with this operation
    	StateChunk chunk = obtainStateChunk(msg.hashkey);
    	chunk.storeStateMessage(msg, this.dst);
        
        // Process chunk
        this.threadPool.submit(chunk);
        
        /*PutAllflowsMessage putAllflows = new PutAllflowsMessage();
        putAllflows.id = msg.id;
        putAllflows.hashkey = msg.hashkey;
        putAllflows.state = msg.state;

        try
        { this.dst.getStateChannel().sendMessage(putAllflows); }
        catch(Exception e)
        { 
            log.error(e.toString());
            this.fail();
        }*/
    }

    @Override
    public void receivePutPerflowAck(PutPerflowAckMessage msg) 
    {
        StateChunk chunk = obtainStateChunk(msg.hashkey);

        this.putPerflowAcks++;

        // Indicate state chunk has been acked
        chunk.receivedPutAck();

        checkIfFinished();
    }
    
    @Override
    public void receivePutMultiflowAck(PutMultiflowAckMessage msg) 
    {
        StateChunk chunk = obtainStateChunk(msg.hashkey);

        this.putMultiflowAcks++;

        // Indicate state chunk has been acked
        chunk.receivedPutAck();

        checkIfFinished();
    }

    @Override
    public void receivePutAllflowsAck(PutAllflowsAckMessage msg) 
    {
        StateChunk chunk = obtainStateChunk(msg.hashkey);

        this.putAllflowsAcks++;

        // Indicate state chunk has been acked
        chunk.receivedPutAck();

        checkIfFinished();
    }
    
    @Override
    public void receiveReprocess(ReprocessMessage msg, Middlebox from) 
    {	
        log.warn("Events received for Copy operation");
        assert false; // Should not receive any events for copy operation
    }

    @Override
    public void receiveGetPerflowAck(GetPerflowAckMessage msg) 
    {
        this.getPerflowAcked = true;
        this.getPerflowCount = msg.count;
        checkIfFinished();
    }

    @Override
    public void receiveGetMultiflowAck(GetMultiflowAckMessage msg) 
    {
        this.getMultiflowAcked = true;
        this.getMultiflowCount = msg.count;
        checkIfFinished();
    }

    @Override
    public void receiveGetAllflowsAck(GetAllflowsAckMessage msg) 
    {
        this.getAllflowsAcked = true;
        this.getAllflowsCount = msg.count;
        checkIfFinished();
	}

    private void checkIfFinished()
    {    	
    	boolean completed = false;
    	
    	switch (this.scope)
		{
		case PERFLOW:
			completed = (this.getPerflowAcked 
					&& (this.putPerflowAcks == this.getPerflowCount));
			break;
		
		case MULTIFLOW:
			completed = (this.getMultiflowAcked 
					&& (this.putMultiflowAcks == this.getMultiflowCount));
    		break;
		
		case ALLFLOWS:
			completed = (this.getAllflowsAcked 
					&& (this.putAllflowsAcks == this.getAllflowsCount));
			break;
		
		case PF_MF:
			completed = (this.getPerflowAcked && this.getMultiflowAcked && 
					(this.putPerflowAcks == this.getPerflowCount) 
					&& (this.putMultiflowAcks == this.getMultiflowCount));
			break;

		case PF_AF:
			completed = (this.getPerflowAcked && this.getAllflowsAcked && 
					(this.putPerflowAcks == this.getPerflowCount) 
					&& (this.putAllflowsAcks == this.getAllflowsCount));
			break;

		case MF_AF:
			completed = (this.getMultiflowAcked && this.getAllflowsAcked && 
					(this.putMultiflowAcks == this.getMultiflowCount)
					&& (this.putAllflowsAcks == this.getAllflowsCount));
			break;
		
		case PF_MF_AF:
			completed = (this.getPerflowAcked && this.getMultiflowAcked 
					&& this.getAllflowsAcked 
					&& (this.putPerflowAcks == this.getPerflowCount) 
					&& (this.putMultiflowAcks == this.getMultiflowCount) 
					&& (this.putAllflowsAcks == this.getAllflowsCount));
			break;

		default:
			log.error("Scope needs to be 'PERFLOW', 'MULTIFLOW', 'ALLFLOWS', 'PF_MF', 'PF_AF', 'MF_AF' or 'PF_MF_AF'");
			break;
		}
    	
    	if (completed)
    	{
            log.info("End Copy");
            long copyEnd = System.currentTimeMillis();
			long copyTime = copyEnd - this.copyStart;
			log.info(String.format("[COPY_TIME] elapse=%d, start=%d, end=%d",
					copyTime, this.copyStart, copyEnd));
            this.finish();
        }
    }

}
