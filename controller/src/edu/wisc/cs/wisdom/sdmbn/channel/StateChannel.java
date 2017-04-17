package edu.wisc.cs.wisdom.sdmbn.channel;

import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.channel.Channel;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.gson.JsonSyntaxException;

import edu.wisc.cs.wisdom.sdmbn.core.SdmbnManager;
import edu.wisc.cs.wisdom.sdmbn.json.AllFieldsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.DelMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.DelMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.DelPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.DelPerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.DisableEventsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.EnableEventsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.ErrorMessage;
import edu.wisc.cs.wisdom.sdmbn.json.EventsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetConfigAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetConfigMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.Message;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;
import edu.wisc.cs.wisdom.sdmbn.json.PutAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutConfigMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutPerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateConfigMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StatePerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.SynMessage;
import edu.wisc.cs.wisdom.sdmbn.operation.Operation;

/**
 * describes the connection with a specific MB; place where actions are send to
 * MBs and messages are received from them
 * */
public class StateChannel extends BaseChannel 
{
	private static Logger log = 
		LoggerFactory.getLogger("sdmbn."+StateChannel.class.getSimpleName());
	
	public StateChannel(Channel channel, SdmbnManager sdmbnManager) 
	{
		super(channel, sdmbnManager);	
		log.info("Connection established with: {}", channel);
	}
		
	protected void processMessage(String line) throws MessageException
	{
		Message msg = null;
		try
        { msg = gson.fromJson(line, AllFieldsMessage.class).cast(); }
		catch(MessageException e)
		{
			log.error(e.toString());
			return;
		}
        catch(JsonSyntaxException e)
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

		if (msg instanceof SynMessage) 
		{
			SynMessage syn = (SynMessage)msg;
			log.info("[SDMBN] - SYN from {}.{}", syn.host, syn.pid);
			this.host = syn.host;
			this.pid = syn.pid;
			sdmbnManager.channelConnected(this);
		}
		else if (msg instanceof StatePerflowMessage) 
		{ op.receiveStatePerflow((StatePerflowMessage)msg); }
		else if (msg instanceof StateMultiflowMessage) 
		{ op.receiveStateMultiflow((StateMultiflowMessage)msg); }
		else if (msg instanceof StateConfigMessage) 
		{ op.receiveStateConfig((StateConfigMessage)msg); }
		else if (msg instanceof GetPerflowAckMessage) 
		{ op.receiveGetPerflowAck((GetPerflowAckMessage)msg); }
		else if (msg instanceof GetMultiflowAckMessage) 
		{ op.receiveGetMultiflowAck((GetMultiflowAckMessage)msg); }
		else if (msg instanceof GetAllflowsAckMessage) 
		{ op.receiveGetAllflowsAck((GetAllflowsAckMessage)msg); }
		else if (msg instanceof GetConfigAckMessage) 
		{ op.receiveGetConfigAck((GetConfigAckMessage)msg); }
		else if (msg instanceof DelPerflowAckMessage) 
		{ op.receiveDelPerflowAck((DelPerflowAckMessage)msg); }
		else if (msg instanceof DelMultiflowAckMessage) 
		{ op.receiveDelMultiflowAck((DelMultiflowAckMessage)msg); }
		else if (msg instanceof EventsAckMessage) 
		{
			EventsAckMessage eventsAck = (EventsAckMessage)msg;
			log.debug("Got events ack: "+msg.toString());
			op.receiveEventsAck(eventsAck, this.mb);
		}
		else if (msg instanceof ErrorMessage) 
		{
			log.error("Received error message from "+this.toString()+": "
					+((ErrorMessage)msg).cause); 
		}
		else
		{ throw new MessageException("Unexpected message type: "+msg.type, msg); }
	}

	public void sendMessage(Message msg) throws MessageException 
	{
		String json = null;
		if (msg instanceof GetPerflowMessage)
		{ json = gson.toJson((GetPerflowMessage)msg); }
		else if (msg instanceof GetMultiflowMessage)
		{ json = gson.toJson((GetMultiflowMessage)msg); }
		else if (msg instanceof GetAllflowsMessage)
		{ json = gson.toJson((GetAllflowsMessage)msg); }
		else if (msg instanceof GetConfigMessage)
		{ json = gson.toJson((GetConfigMessage)msg); }
		else if (msg instanceof PutPerflowMessage)
		{ json = gson.toJson((PutPerflowMessage)msg); }
		else if (msg instanceof PutMultiflowMessage)
		{ json = gson.toJson((PutMultiflowMessage)msg); }
		else if (msg instanceof PutAllflowsMessage)
		{ json = gson.toJson((PutAllflowsMessage)msg); }
		else if (msg instanceof PutConfigMessage)
		{ json = gson.toJson((PutConfigMessage)msg); }
		else if (msg instanceof DelPerflowMessage)
		{ json = gson.toJson((DelPerflowMessage)msg); }
		else if (msg instanceof DelMultiflowMessage)
		{ json = gson.toJson((DelMultiflowMessage)msg); }
		else if (msg instanceof EnableEventsMessage)
		{ json = gson.toJson((EnableEventsMessage)msg); }
		else if (msg instanceof DisableEventsMessage)
		{ json = gson.toJson((DisableEventsMessage)msg); }
		else
		{ throw new MessageException("Unexpected message type: "+msg.type, msg); }
		
			
		ChannelBuffer buf = ChannelBuffers.buffer(4+json.length());
		buf.writeInt(json.length());
		buf.writeBytes(json.getBytes());
		channel.write(buf);
	
		/*(if (os.checkError()) 
		{ throw new MessageException("Failed to send message", msg); }*/
	}
}
