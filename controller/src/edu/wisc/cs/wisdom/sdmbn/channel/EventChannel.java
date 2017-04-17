package edu.wisc.cs.wisdom.sdmbn.channel;

import org.jboss.netty.channel.Channel;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.core.SdmbnManager;
import edu.wisc.cs.wisdom.sdmbn.json.AllFieldsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.Message;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;
import edu.wisc.cs.wisdom.sdmbn.json.PutAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutConfigAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.ReprocessMessage;
import edu.wisc.cs.wisdom.sdmbn.json.SynMessage;
import edu.wisc.cs.wisdom.sdmbn.operation.Operation;

/** treat events from a specific MB here */
public class EventChannel extends BaseChannel
{			
	private static Logger log = 
		LoggerFactory.getLogger("sdmbn."+EventChannel.class.getSimpleName());
	
	public EventChannel(Channel channel, SdmbnManager sdmbnManager)
	{
		super(channel, sdmbnManager);	
		log.info("Connection established with: {}", channel);
	}
			
	protected void processMessage(String line) throws MessageException 
	{
		Message msg = null;
        try
        { msg = gson.fromJson(line, AllFieldsMessage.class).cast(); }
        catch(Exception e)
        { 
            log.error(String.format("Unable to parse line: %s", line));
            return;
        }

		Operation op = null;
		if (msg.id > 0)
		{ 
			op = sdmbnManager.lookupOperation(msg.id);	
			if (op == null)
			{ throw new MessageException("Unknown operation: "+msg.id, msg); }
			
			if (op.isFailed())
			{
				log.info(String.format("Discarded %s message for failed operation %d", 
						msg.type, msg.id));
				return;
			}
		}
		
		if (msg instanceof SynMessage) {
			SynMessage syn = (SynMessage)msg;
			log.info("[SDMBN] - SYN from {}.{}", syn.host,syn.pid);
			this.host = syn.host;
			this.pid = syn.pid;
			sdmbnManager.channelConnected(this);
		}
		else if (msg instanceof ReprocessMessage) 
		{ op.receiveReprocess((ReprocessMessage)msg, this.mb); }
		else if (msg instanceof PutPerflowAckMessage) 
		{ op.receivePutPerflowAck((PutPerflowAckMessage)msg); }
		else if (msg instanceof PutMultiflowAckMessage) 
		{ op.receivePutMultiflowAck((PutMultiflowAckMessage)msg); }
		else if (msg instanceof PutAllflowsAckMessage) 
		{ op.receivePutAllflowsAck((PutAllflowsAckMessage)msg); }
		else if (msg instanceof PutConfigAckMessage) 
		{ op.receivePutConfigAck((PutConfigAckMessage)msg); }
		else
		{ throw new MessageException("Unexpected message type: "+msg.type, msg); }
	}

	@Override
	public void sendMessage(Message msg) throws MessageException 
	{
		throw new MessageException("Unexpected message type: "+msg.type, msg);
	}
}
