package edu.wisc.cs.wisdom.sdmbn.utils;

import java.util.Arrays;
import java.nio.ByteBuffer;

import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;

public class PacketUtil 
{
	public final static byte FLAG_NONE = 0; 
	public final static byte FLAG_DO_NOT_BUFFER = (1 << 2); 
	public final static byte FLAG_DO_NOT_DROP = (1 << 3);

	private final static int ETH_HDR_LEN = 14;
	
	public static byte[] setFlag(byte[] packet, byte flag)
	{		
	    ByteBuffer bb = ByteBuffer.wrap(packet, 0, packet.length);
        bb.get(new byte[Ethernet.DATALAYER_ADDRESS_LENGTH]); // Skip dst MAC
        bb.get(new byte[Ethernet.DATALAYER_ADDRESS_LENGTH]); // Skip src MAC
        short etherType = bb.getShort(); // Get EtherType

        if (Ethernet.TYPE_IPv4 == etherType)
        {  
            bb.get(); // Skip Ver/IHL
            bb.get(); // Skip ToS
            short ipLen = bb.getShort(); // Get IP length

            int actualLen = ETH_HDR_LEN + ipLen;
            System.out.println(String.format("len=%d,ipLen=%d,actualLen=%d",
                        packet.length, ipLen, actualLen));

            byte[] actualPacket = packet;
            if (actualLen < packet.length)
            { actualPacket = Arrays.copyOf(packet, actualLen); }
            Ethernet ethPacket = new Ethernet();
            ethPacket.deserialize(actualPacket, 0, actualLen);
            
            IPv4 ipPacket = (IPv4)ethPacket.getPayload();
            ipPacket.setDiffServ(flag);
            ipPacket.resetChecksum();
            return ethPacket.serialize();
        }

	    return null;
	}
	
}
