package edu.wisc.cs.wisdom.sdmbn.core;

import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.Callable;

import net.floodlightcontroller.core.IOFSwitch;

import org.openflow.protocol.OFPacketOut;
import org.openflow.protocol.OFPort;
import org.openflow.protocol.action.OFAction;
import org.openflow.protocol.action.OFActionOutput;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;
import edu.wisc.cs.wisdom.sdmbn.json.ReprocessMessage;
import edu.wisc.cs.wisdom.sdmbn.utils.PacketUtil;

public class ReprocessEvent implements Callable<Boolean>
{
	//private ReprocessMessage msg;
	private Middlebox dst;
    private static int eventOutCount = 0;
    public int eventId;
    //private byte[] decodedPacket;
    private String packet;
    private boolean sendDirect;
    private int hashkey;
    private String header;
	
	private static Logger log = LoggerFactory.getLogger("sdmbn.ReprocessEvent");
	
	public ReprocessEvent(ReprocessMessage msg, Middlebox dst, boolean sendDirect)
	{
		//this.msg = msg;
		this.dst = dst;
		this.eventId = -1;
		this.packet = msg.packet;
		this.sendDirect = sendDirect;
		if (this.sendDirect)
		{
			this.hashkey = msg.hashkey;
			this.header = msg.header;
		}
		
		//this.decodedPacket = ReprocessEvent.base64_decode(msg.packet);
	}
	
	public ReprocessEvent(ReprocessMessage msg, Middlebox dst)
	{ this(msg, dst, false); }
	
	@Override
	public Boolean call()
	{
        byte[] decodedPacket = ReprocessEvent.base64_decode(this.packet);
        // Log an error if the packet is of an unexpectedly large size
        if (decodedPacket.length > 1514)        
        { log.info(String.format("pktLength=%d", decodedPacket.length)); }
        
        // Mark with do-not-buffer flag
        decodedPacket = PacketUtil.setFlag(decodedPacket, 
        		PacketUtil.FLAG_DO_NOT_BUFFER);   
        
		if (this.sendDirect)
		{ this.sendDirect(decodedPacket); }
		else
		{ this.sendViaSwitch(decodedPacket); }
		return null;
	}
	
	@Deprecated
	public void process()
	{ this.call(); }
	
	/**
	 * Sends the redirected packet to the switch to which the middlebox is 
	 * connected and has the switch forward it to the middlebox 
	 */
	public void sendViaSwitch(byte[] decodedPacket) 
	{
		//TODO: we should forward the packet over the control channel
		// Create packet out message 
        OFPacketOut packetOutMessage = new OFPacketOut();
		packetOutMessage.setBufferId(OFPacketOut.BUFFER_ID_NONE);
		packetOutMessage.setInPort(OFPort.OFPP_NONE);

		// Set actions
        packetOutMessage.setActions(Arrays.asList(new OFAction[] {
                    new OFActionOutput((short) dst.getSwitchPort()) } ));
        packetOutMessage.setActionsLength((short)OFActionOutput.MINIMUM_LENGTH);     
        
        // Set packet
        packetOutMessage.setPacketData(decodedPacket);
        
        // Set the length of the packet out message
        packetOutMessage.setLength((short)(decodedPacket.length
                    + OFPacketOut.MINIMUM_LENGTH 
                    + OFActionOutput.MINIMUM_LENGTH));  
               		
        // Send packet out message to the switch
        IOFSwitch sw = dst.getSwitch();
        try 
        {
        	//SdmbnManager.counterStore.updatePktOutFMCounterStore(sw, packetOutMessage);
            sw.write(packetOutMessage, null);
            sw.flush();
            ReprocessEvent.eventOutCount++;
            log.info(String.format("[CONT_EXIT] %d", this.eventId));
            /*try {
            	MessageDigest md = MessageDigest.getInstance("MD5");
               	md.update(decodedPacket, 26, decodedPacket.length - 26);
               
               	BigInteger hash = new BigInteger(1, md.digest());
            	String hd = hash.toString(16);
            	while(hd.length() < 32) {hd = "0" + hd;	}
            	
            	log.info(String.format("[CONT_EXIT] %d : %s", this.eventId, hd));*
            	
            } catch (Exception e) {
            	log.debug("[LASTMIN] MD5 algorithm not found");
            }*/
        }
        catch (IOException e) 
        {
            log.error("Failed to write {} to switch {}: {}", 
            		new Object[]{ packetOutMessage, sw, e });
        }
	}
	
	public void sendDirect(byte[] decodedPacket)
	{
		ReprocessMessage msg = new ReprocessMessage();
		msg.hashkey = this.hashkey;
		msg.header = this.header;
		msg.packet = ReprocessEvent.base64_encode(decodedPacket);
		
		try
		{
			this.dst.getEventChannel().sendMessage(msg);
			ReprocessEvent.eventOutCount++;
            log.info(String.format("[CONT_EXIT_DIRECT] %d", this.eventId));
		}
		catch(MessageException e)
		{ log.error(e.toString()); }
	}
		
	public static byte[] base64_decode(String encodedPkt) 
	{
		byte [] decodedPacket = new byte[encodedPkt.length() / 2];
		for (int i = 0; i < encodedPkt.length()/2; i++)
		{
			char ch = encodedPkt.charAt(i*2);
			byte lower = 0;
			if (ch >= 'A' && ch <= 'Z')
				lower = (byte)(ch - 'A');                                       
			if (ch >= 'a' && ch <= 'z')                                
				lower = (byte)(ch + 26 - 'a');                                  
			if (ch >= '0' && ch <= '9')                                
				lower = (byte)(ch + 52 - '0');                                  
			if (ch == '+')                                               
				lower = 62;                                                 
			if (ch == '/')
				lower = 63;
			ch = encodedPkt.charAt(i*2+1);
			byte upper = 0;
			if (ch >= 'A' && ch <= 'Z')
				upper = (byte)(ch - 'A');                                       
			if (ch >= 'a' && ch <= 'z')                                
				upper = (byte)(ch + 26 - 'a');                                  
			if (ch >= '0' && ch <= '9')                                
				upper = (byte)(ch + 52 - '0');                                  
			if (ch == '+')                                               
				upper = 62;                                                 
			if (ch == '/')
				upper = 63;
			byte full = (byte)((upper << 4) | lower);
			decodedPacket[i] = full;
		}
		
		return decodedPacket;
	}
	
	public static String base64_encode(byte[] decodedPkt)
	{
		if (null == decodedPkt || decodedPkt.length < 1)
		{ return null; }

		int size = decodedPkt.length;
		char[] encodedPacket = new char[size*2+1];
		
	    int ptrResult = 0;
	    int ptrBlob = 0;
	    while (size > 0)
	    {
	        byte lower = (byte)(decodedPkt[ptrBlob] & 0x0F);
	        byte upper = (byte)((decodedPkt[ptrBlob] & 0xF0) >> 4);
	        encodedPacket[ptrResult] = base64_encode_bits(lower);
	        ptrResult++;
	        encodedPacket[ptrResult] = base64_encode_bits(upper);
	        ptrResult++;
	        ptrBlob++;
	        size += -1;
	    }
	    encodedPacket[ptrResult] = '\0';
	    return new String(encodedPacket);
	}
	
	private static char base64_encode_bits(byte data)
	{
	    if (data < 26)
	        return (char)(data + 'A');
	    if (data < 52)
	        return (char)(data - 26 + 'a');
	    if (data < 62)
	        return (char)(data - 52 + '0');
	    if (data == 62)
	        return '+';
	    if (data == 63)
	        return '/';
	    return 0;
	}
}
