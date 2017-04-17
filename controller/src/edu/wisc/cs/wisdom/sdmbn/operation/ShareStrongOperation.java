package edu.wisc.cs.wisdom.sdmbn.operation;

import java.util.Arrays;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map.Entry;
import java.util.NoSuchElementException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.openflow.protocol.OFMatch;
import org.openflow.protocol.OFPacketOut;
import org.openflow.protocol.action.OFAction;
import org.openflow.protocol.action.OFActionOutput;

import edu.wisc.cs.wisdom.sdmbn.Parameters.*;
import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.core.ReprocessEvent;
import edu.wisc.cs.wisdom.sdmbn.core.StateChunk;
import edu.wisc.cs.wisdom.sdmbn.json.EnableEventsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.EventsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.Message;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;
import edu.wisc.cs.wisdom.sdmbn.json.PutAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.ReprocessMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StatePerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.utils.PacketUtil;

public class ShareStrongOperation extends Operation 
{
    private List<Middlebox> instances;
    private PerflowKey key; /* We want to share states on only this key */
    private Scope scope; /* per or multi flow */
    private boolean isSyncFinished;
    private boolean isFirstStateRcvd;

    private boolean getPerflowAcked;
    private boolean getMultiflowAcked;
    private boolean getAllflowsAcked;
    private volatile int getPerflowCount;
    private volatile int getMultiflowCount;
    private volatile int getAllflowsCount;
    private volatile int putPerflowAcks;
    private volatile int putMultiflowAcks;
    private volatile int putAllflowsAcks;
    private Lock eventLock = new ReentrantLock();
    private Lock finishCheckLock = new ReentrantLock();

    private Middlebox currentSource;
    private List<Middlebox> instancesExcludingSource;
    private int eventAckCount; /* To keep track of acks for enable events from instances */
    private LinkedHashMap<String, Middlebox> originalEvents; /* String is the packet, middlebox is the one from where it came */
	private byte[] lastPacket; /* to remember the last packet sent out of the controller for processing at originating middlebox.
	Once we receive and event after processing this packet, we send the next one out. */
	private boolean callFromReprocess; /* hack --- to avoid a race condition */

    private int nextInstIndex; /* the next instance in list of NF */
    
    private ExecutorService threadPool;

    public ShareStrongOperation(IOperationManager manager, List<Middlebox> instances, 
            PerflowKey key, Scope scope)
    {
        super(manager);
        this.instances = instances;
        this.key = key;
        this.scope = scope;
        
        this.isSyncFinished = false;
        this.getPerflowAcked = false;
        this.getMultiflowAcked = false;
        this.getAllflowsAcked = false;
        this.getPerflowCount = 0;
        this.getMultiflowCount = 0;
        this.getAllflowsCount = 0;
        this.putPerflowAcks = 0;
        this.putMultiflowAcks = 0;
        this.putAllflowsAcks = 0;
        this.isFirstStateRcvd = false;
        this.currentSource = null;
        this.instancesExcludingSource = null;
        this.eventAckCount = 0;

        this.originalEvents = new LinkedHashMap<String, Middlebox>();
        this.lastPacket = null;
		this.callFromReprocess = false;

        this.nextInstIndex = 0;
        
        this.threadPool = Executors.newCachedThreadPool(); 
    }
    
    @Override
    public int execute()
    {	
    	enableEvents(); // done once, in application called for all flow
        return this.getId();
    }
    
    private void enableEvents()
    {
    	//first enable events with drop parameter on all NFs
    	log.info("Enabling events with drop flag on all NFs");
    	EnableEventsMessage enableEvents = new EnableEventsMessage();
		enableEvents.id = this.getId();
		enableEvents.key = this.key;
		enableEvents.action = Message.CONSTANT_ACTION_DROP;
    	for(Middlebox inst : this.instances) 
        {
			try
			{ inst.getStateChannel().sendMessage(enableEvents); }
			catch(MessageException e)
			{ 
				this.fail();
				log.error("Failed to request Drop on instance " + inst.getHost()); 
			}
			log.info("[SIGSHARE] Requested Drop on instance " + inst.getHost());
			System.out.println(System.currentTimeMillis());
    	}
    	log.info("[SIGSHARE] Waiting for enableEvent acks from all NFs");
    }
    
    
    @Override
	public void receiveEventsAck(EventsAckMessage msg, Middlebox from)
	{
    	this.eventAckCount++;
    	if (this.eventAckCount == this.instances.size())
        { 
            log.debug("[SIGSHARE] Bring all NFs to a synchronized state");
    	    this.syncAllInstances();
        }
	}
    
    
    private synchronized void syncAllInstances()
    {
        if (this.nextInstIndex == this.instances.size())
        { 
        	this.isSyncFinished = true;
        	log.info("sync complete, dequeue event");
    		dequeueEvent();
        	return; 
        }

        Middlebox inst = this.instances.get(this.nextInstIndex);
        this.syncFromInstance(inst);
        
        this.nextInstIndex++;
    }

    private int syncFromInstance(Middlebox inst)
    {
    	this.getPerflowAcked = false;
    	this.getMultiflowAcked = false;
    	this.getAllflowsAcked = false;
		this.getPerflowCount = 100000; // a large number
		this.getMultiflowCount = 100000;
		this.getAllflowsCount = 100000;
        this.putPerflowAcks = 0;
        this.putMultiflowAcks = 0;
        this.putAllflowsAcks = 0;
        this.currentSource = inst;
        this.instancesExcludingSource = 
			new ArrayList<Middlebox>(this.instances);
        this.instancesExcludingSource.remove(this.currentSource);
        
        GetPerflowMessage getPerflow;
        GetMultiflowMessage getMultiflow;
        GetAllflowsMessage getAllflows;
        switch (this.scope)
        {
        case PERFLOW:
			           	getPerflow = new GetPerflowMessage();
			            getPerflow.id = this.getId();
			            getPerflow.key = this.key;
			            getPerflow.raiseEvents = 0;
			            try
			            { inst.getStateChannel().sendMessage(getPerflow); }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get perflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			            break;
        
        case MULTIFLOW:
			            getMultiflow = new GetMultiflowMessage();
			            getMultiflow.id = this.getId();
			            getMultiflow.key = this.key;
			            try	
			            { 
			            	inst.getStateChannel().sendMessage(getMultiflow); 
			            	log.info("Issued get MultiFlow");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get multiflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			            break;
    
        case ALLFLOWS:
						getAllflows = new GetAllflowsMessage();
						getAllflows.id = this.getId();
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getAllflows); 
			            	log.info("Issued get Allflows");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Allflows to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		break;

		case PF_MF:
						getPerflow = new GetPerflowMessage();
						getPerflow.id = this.getId();
						getPerflow.key = this.key;
						getPerflow.raiseEvents = 0;
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getPerflow); 
			            	log.info("Issued get Perflow");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Perflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		getMultiflow = new GetMultiflowMessage();
						getMultiflow.id = this.getId();
						getMultiflow.key = this.key;
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getMultiflow); 
			            	log.info("Issued get Mutiflow");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Multiflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		break;
		    		
		case PF_AF:
						getPerflow = new GetPerflowMessage();
						getPerflow.id = this.getId();
						getPerflow.key = this.key;
						getPerflow.raiseEvents = 0;
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getPerflow); 
			            	log.info("Issued get Perflow");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Perflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		getAllflows = new GetAllflowsMessage();
						getAllflows.id = this.getId();
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getAllflows); 
			            	log.info("Issued get Allflows");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Allflows to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		break;
		    		
		case MF_AF:
						getMultiflow = new GetMultiflowMessage();
						getMultiflow.id = this.getId();
						getMultiflow.key = this.key;
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getMultiflow); 
			            	log.info("Issued get Perflow");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Perflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		getAllflows = new GetAllflowsMessage();
						getAllflows.id = this.getId();
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getAllflows); 
			            	log.info("Issued get Allflows");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Allflows to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		break;
		
		case PF_MF_AF:
						getPerflow = new GetPerflowMessage();
						getPerflow.id = this.getId();
						getPerflow.key = this.key;
						getPerflow.raiseEvents = 0;
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getPerflow); 
			            	log.info("Issued get Perflow");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Perflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		getMultiflow = new GetMultiflowMessage();
						getMultiflow.id = this.getId();
						getMultiflow.key = this.key;
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getMultiflow); 
			            	log.info("Issued get Multiflow");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Multiflow to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		getAllflows = new GetAllflowsMessage();
						getAllflows.id = this.getId();
						try	
			            { 
			            	inst.getStateChannel().sendMessage(getAllflows); 
			            	log.info("Issued get Allflows");
			            }
			            catch(MessageException e)
			            { 
			                log.error("Failed to send get Allflows to "+inst.toString());
							this.fail();
			                return -1;
			            }
			    		break;
		
		default:
						log.error("Scope needs to be 'PERFLOW', 'MULTIFLOW', 'ALLFLOWS', 'PF_MF', 'PF_AF', 'MF_AF' or 'PF_MF_AF'");
						return -1;
        }
        return 0;
    }

    @Override
    public void receiveStatePerflow(StatePerflowMessage msg) 
    {
        if(this.isFirstStateRcvd == false) {
            log.info("Share Start ({})", this.getId());
            this.isFirstStateRcvd = true;
        }

        // Add state chunk to list of state associated with this operation
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        chunk.storeStateMessage(msg, this.instancesExcludingSource);
        
        // Process chunk
        this.threadPool.submit(chunk);

        /*PutPerflowMessage putPerflow = new PutPerflowMessage();
        putPerflow.id = msg.id;
        putPerflow.seq = msg.seq;
        putPerflow.key = msg.key;
        putPerflow.hashkey = msg.hashkey;
        putPerflow.state = msg.state;
        for(Middlebox inst : this.instances) {
        	if (currentSource.equals(inst))
        		continue;
        	try
            { inst.getStateChannel().sendMessage(putPerflow); }
            catch(Exception e)
            {
                log.error(e.toString()); 
                this.fail();
            }
        }*/
    }

    @Override
    public void receiveStateMultiflow(StateMultiflowMessage msg) 
    { 
        	if(this.isFirstStateRcvd == false) {
            	log.info("Share Start ({})", this.getId());
            	this.isFirstStateRcvd = true;
        	}
        // Add state chunk to list of state associated with this operation
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        chunk.storeStateMessage(msg, this.instancesExcludingSource);
        
        // Process chunk
        this.threadPool.submit(chunk);
        
        /*PutMultiflowMessage putMultiflow = new PutMultiflowMessage();
        putMultiflow.id = msg.id;
        putMultiflow.seq = msg.seq;
        putMultiflow.key = msg.key;
        putMultiflow.hashkey = msg.hashkey;
        putMultiflow.state = msg.state;
        
        //log.debug(String.format("[SIGSHARE] Put for %d", msg.hashkey));

        for(Middlebox inst : this.instances) {
        	if (currentSource.equals(inst))
        		continue;
	        try
	        { inst.getStateChannel().sendMessage(putMultiflow); }
	        catch(Exception e)
	        { 
	            log.error(e.toString());
	            this.fail();
	        }
        }*/
    }

    @Override
    public void receiveStateAllflows(StateAllflowsMessage msg) 
    { 
    	if(this.isFirstStateRcvd == false) 
    	{
        	log.info("Share Start ({})", this.getId());
        	this.isFirstStateRcvd = true;
    	}
        // Add state chunk to list of state associated with this operation
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        chunk.storeStateMessage(msg, this.instancesExcludingSource);
        
        // Process chunk
        this.threadPool.submit(chunk);
                
        /*PutAllflowsMessage putAllflows = new PutAllflowsMessage();
        putAllflows.id = msg.id;
        putAllflows.hashkey = msg.hashkey;
        putAllflows.state = msg.state;
        
        //log.debug(String.format("[SIGSHARE] Put for %d", msg.hashkey));

        for(Middlebox inst : this.instances) {
        	if (currentSource.equals(inst))
        		continue;
	        try
	        { inst.getStateChannel().sendMessage(putAllflows); }
	        catch(Exception e)
	        { 
	            log.error(e.toString());
	            this.fail();
	        }
        }*/
    }
    
    @Override
    public void receivePutPerflowAck(PutPerflowAckMessage msg) 
    {
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        this.putPerflowAcks++;
        chunk.incrementAcked();
        // Indicate state chunk has been acked
        if (chunk.numAckedInst() == (this.instances.size()-1) )
        {
        	chunk.receivedPutAck();
        	checkIfInstFinished();
        }
    }

    @Override
    public void receivePutMultiflowAck(PutMultiflowAckMessage msg) 
    {
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        this.putMultiflowAcks++;
        chunk.incrementAcked();
        // Indicate state chunk has been acked
        if (chunk.numAckedInst() == (this.instances.size()-1) )
        {
        	chunk.receivedPutAck();
        	checkIfInstFinished();
        }
    }

    @Override
    public void receivePutAllflowsAck(PutAllflowsAckMessage msg) 
    {
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        this.putAllflowsAcks++;
        chunk.incrementAcked();
        // Indicate state chunk has been acked
        if (chunk.numAckedInst() == (this.instances.size()-1) )
        {
        	chunk.receivedPutAck();
        	checkIfInstFinished();
        }
    }

    
    @Override
    public void receiveGetPerflowAck(GetPerflowAckMessage msg) 
    {
        this.getPerflowAcked = true;
        this.getPerflowCount = msg.count;
        checkIfInstFinished();
    }
    
    @Override
    public void receiveGetMultiflowAck(GetMultiflowAckMessage msg) 
    {
        this.getMultiflowAcked = true;
        this.getMultiflowCount = msg.count;
        checkIfInstFinished();
    }
    
    @Override
    public void receiveGetAllflowsAck(GetAllflowsAckMessage msg) 
    {
        this.getAllflowsAcked = true;
        this.getAllflowsCount = msg.count;
        checkIfInstFinished();
    }
  
    @Override
    public void receiveReprocess(ReprocessMessage msg, Middlebox from) 
    {	
    	byte[] pkt = ReprocessEvent.base64_decode(msg.packet);
    	pkt = PacketUtil.setFlag(pkt, PacketUtil.FLAG_DO_NOT_DROP);
    	String packet = new String(pkt);
    	synchronized(this.originalEvents) {
    		if (this.isSyncFinished == false)
    		{
		    	this.originalEvents.put(packet, from);
    		}
    		else {
    			if (Arrays.equals(this.lastPacket, pkt))
    			{
    				log.debug("RECEIVED DUPLICATE EVENT from " + from);
    				this.originalEvents.remove(packet);
    				OFMatch match = new OFMatch();
    				match.loadFromPacket(pkt, (short)1);
    				PerflowKey key = PerflowKey.fromOFMatch(match);
    				this.key = key;
    				syncFromInstance(from);
    				//dequeueEvent is called automatically from syncAllInstances when state transfer is complete
    			}
    			else
    			{
    				//log.info("[SIGSHARE] RECEIVED ORIGINAL EVENT from " + from);
    				this.originalEvents.put(packet, from);
    				//log.info("[SIGSHARE] Current no. of pkts to be dequeued"+ this.originalEvents.size());
    				eventLock.lock();
    				if (this.callFromReprocess == true)
    				{
    					this.callFromReprocess = false;
    					eventLock.unlock();
    					dequeueEvent();
    				}
    				try {
    					eventLock.unlock();
    				} catch (Exception e) {}
    				
    			}
	    	}
    	}
    }
    
    private void dequeueEvent()
    {
    	synchronized(this.originalEvents) {
    			Entry<String, Middlebox> mb = null;
    			try
    			{
    				mb = this.originalEvents.entrySet().iterator().next();
    			}
    			catch (NoSuchElementException e)
    			{
    				//queue is empty. The next reprocess event should call dequeue
    				this.callFromReprocess = true;
    				return;
    			}
    			String packet = mb.getKey();
    			Middlebox src = mb.getValue();
    			
				log.debug("Sending packet in event back to the originating NF " + src.getHost() + "with No_drop flag");
	       		
	       		byte[] pkt = packet.getBytes();
				//pkt = PacketUtil.setFlag(pkt, PacketUtil.FLAG_DO_NOT_DROP);
				this.lastPacket = pkt;
				OFPacketOut send = new OFPacketOut();
				ArrayList<OFAction> actions = new ArrayList<OFAction>();
		        OFAction outputTo = new OFActionOutput(src.getSwitchPort());
		        actions.add(outputTo);
		        send.setActions(actions);
		        send.setActionsLength((short)OFActionOutput.MINIMUM_LENGTH);
		        send.setLength((short)(OFPacketOut.MINIMUM_LENGTH + send.getActionsLength()));
		        send.setBufferId(OFPacketOut.BUFFER_ID_NONE);

		        // Add packet
		        send.setPacketData(pkt);
		        send.setLength((short)(send.getLength() + pkt.length));
		        try
		        {
		            src.getSwitch().write(send, null);
		            src.getSwitch().flush();
			   }
		        catch (Exception e)
		        {
		        	log.error("Could not flush packet of len "+pkt.length+" to src");
		        	this.fail();
		        	return;
		        }
    			this.currentSource = src;
    			this.instancesExcludingSource = 
    				new ArrayList<Middlebox>(this.instances);
    			this.instancesExcludingSource.remove(this.currentSource);
    	}
    }
    
    
    private void checkIfInstFinished()
    {	
    	switch (this.scope)
    	{
    		case PERFLOW:
			    			finishCheckLock.lock();
			    	        if (this.getPerflowAcked && (this.putPerflowAcks == (this.getPerflowCount * (this.instances.size()-1))))
			    	        {
			    	    		this.getPerflowAcked = false;
			    				finishCheckLock.unlock();
			    				
			    	            this.flushStateChunks();
			    	            syncAllInstances();
			    	        }
			    	    	try {
			    	    		finishCheckLock.unlock();
			    	    	} catch (Exception e) {}
			    	    	break;
			    	   
    		case MULTIFLOW:
    			finishCheckLock.lock();
    	        if (this.getMultiflowAcked && (this.putMultiflowAcks == (this.getMultiflowCount * (this.instances.size()-1))))
    	        {
    	    		this.getMultiflowAcked = false;
    				finishCheckLock.unlock();
    				
    	            this.flushStateChunks();
    	            syncAllInstances();
    	        }
    	    	try {
    	    		finishCheckLock.unlock();
    	    	} catch (Exception e) {}
    	    	break;
    	   
    		case ALLFLOWS:
    			finishCheckLock.lock();
    	        if (this.getAllflowsAcked && (this.putAllflowsAcks == (this.getAllflowsCount * (this.instances.size()-1))))
    	        {
    	    		this.getAllflowsAcked = false;
    				finishCheckLock.unlock();
    				
    	            this.flushStateChunks();
    	            syncAllInstances();
    	        }
    	    	try {
    	    		finishCheckLock.unlock();
    	    	} catch (Exception e) {}
    	    	break;
    	   
    		case PF_MF:
    			finishCheckLock.lock();
    	        if (this.getPerflowAcked && (this.putPerflowAcks == (this.getPerflowCount * (this.instances.size()-1))) &&
    	        		this.getMultiflowAcked && (this.putMultiflowAcks == (this.getMultiflowCount * (this.instances.size()-1))))
    	        {
    	    		this.getPerflowAcked = false;
    	    		this.getMultiflowAcked = false;
    				finishCheckLock.unlock();
    				
    	            this.flushStateChunks();
    	            syncAllInstances();
    	        }
    	    	try {
    	    		finishCheckLock.unlock();
    	    	} catch (Exception e) {}
    	    	break;
    	   
    		case PF_AF:
    			finishCheckLock.lock();
    	        if (this.getPerflowAcked && (this.putPerflowAcks == (this.getPerflowCount * (this.instances.size()-1))) &&
    	        		this.getAllflowsAcked && (this.putAllflowsAcks == (this.getAllflowsCount * (this.instances.size()-1))))
    	        {
    	    		this.getPerflowAcked = false;
    	    		this.getAllflowsAcked = false;
    				finishCheckLock.unlock();
    				
    	            this.flushStateChunks();
    	            syncAllInstances();
    	        }
    	    	try {
    	    		finishCheckLock.unlock();
    	    	} catch (Exception e) {}
    	    	break;
    	   
    		case MF_AF:
    			finishCheckLock.lock();
    	        if (this.getMultiflowAcked && (this.putMultiflowAcks == (this.getMultiflowCount * (this.instances.size()-1))) &&
    	        		this.getAllflowsAcked && (this.putAllflowsAcks == (this.getAllflowsCount * (this.instances.size()-1))))
    	        {
    	    		this.getMultiflowAcked = false;
    	    		this.getAllflowsAcked = false;
    				finishCheckLock.unlock();
    				
    	            this.flushStateChunks();
    	            syncAllInstances();
    	        }
    	    	try {
    	    		finishCheckLock.unlock();
    	    	} catch (Exception e) {}
    	    	break;
    		
    		case PF_MF_AF:
    			finishCheckLock.lock();
    	        if (this.getPerflowAcked && (this.putPerflowAcks == (this.getPerflowCount * (this.instances.size()-1))) &&
    	        		this.getMultiflowAcked && (this.putMultiflowAcks == (this.getMultiflowCount * (this.instances.size()-1))) &&
    	        		this.getAllflowsAcked && (this.putAllflowsAcks == (this.getAllflowsCount * (this.instances.size()-1))))
    	        {
    	    		this.getPerflowAcked = false;
    	    		this.getMultiflowAcked = false;
    	    		this.getAllflowsAcked = false;
    				finishCheckLock.unlock();
    				
    	            this.flushStateChunks();
    	            syncAllInstances();
    	        }
    	    	try {
    	    		finishCheckLock.unlock();
    	    	} catch (Exception e) {}
    	    	break;
    	   
    	}
    	
    }

}
