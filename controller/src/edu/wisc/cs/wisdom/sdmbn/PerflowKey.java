package edu.wisc.cs.wisdom.sdmbn;

import java.util.ArrayList;
import java.util.List;

import org.openflow.protocol.OFMatch;

import net.floodlightcontroller.packet.IPv4;

public class PerflowKey
{	
	private String nw_src;
	private String nw_dst;
	private Integer nw_src_mask;
	private Integer nw_dst_mask;
	private Short tp_src;
	private Short tp_dst;
	private byte tp_flip;
	private Short dl_type;
	private Byte nw_proto;
	
	public PerflowKey()
	{
		this.nw_src = null;
		this.nw_dst = null;
		this.nw_src_mask = null;
		this.nw_dst_mask = null;
		this.tp_src = null;
		this.tp_dst = null;
		this.tp_flip = 0;
		this.dl_type = null;
		this.nw_proto = null;
	}
	
	public void setNwSrc(String nw_src)
	{ this.nw_src = nw_src; }
	
	public boolean hasNwSrc()
	{ return (this.nw_src != null); }
	
	public String getNwSrc()
	{ return this.nw_src; }
	
	public void setNwDst(String nw_dst)
	{ this.nw_dst = nw_dst; }
	
	public boolean hasNwDst()
	{ return (this.nw_dst != null); }
	
	public String getNwDst()
	{ return this.nw_dst; }
	
	public void setNwSrcMask(Integer nw_src_mask)
	{ this.nw_src_mask = nw_src_mask; }
	
	public boolean hasNwSrcMask()
	{ return (this.nw_src_mask != null); }
	
	public Integer getNwSrcMask()
	{ return this.nw_src_mask; }
	
	public void setNwDstMask(Integer nw_dst_mask)
	{ this.nw_dst_mask = nw_dst_mask; }
	
	public boolean hasNwDstMask()
	{ return (this.nw_dst_mask != null); }
	
	public Integer getNwDstMask()
	{ return this.nw_dst_mask; }
	
	public void setTpSrc(Short tp_src)
	{ this.tp_src = tp_src; }
	
	public boolean hasTpSrc()
	{ return (this.tp_src != null); }
	
	public short getTpSrc()
	{ return this.tp_src; }
	
	public void setTpDst(Short tp_dst)
	{ this.tp_dst = tp_dst; }
	
	public boolean hasTpDst()
	{ return (this.tp_dst != null); }
	
	public short getTpDst()
	{ return this.tp_dst; }
	
	public void setTpFlip(boolean tp_flip)
	{ this.tp_flip = (byte)(tp_flip ? 1 : 0); }
	
	public boolean getTpFlip()
	{ return (this.tp_flip == 1); }
	
	public void setDlType(Short dl_type)
	{ this.dl_type = dl_type; }
	
	public boolean hasDlType()
	{ return (this.dl_type != null); }
	
	public short getDlType()
	{ return this.dl_type; }
	
	public void setNwProto(byte nw_proto)
	{ this.nw_proto = nw_proto; }
	
	public boolean hasNwProto()
	{ return (this.nw_proto != null); }
	
	public byte getNwProto()
	{ return this.nw_proto; }
	
	public String toString()
	{
		ArrayList<String> fields = new ArrayList<String>();
		if (this.hasNwSrc())
		{ fields.add("nw_src=\""+nw_src+"\""); }
		if (this.hasNwSrcMask())
		{ fields.add("nw_src_mask="+nw_src_mask); }
		if (this.hasNwDst())
		{ fields.add("nw_dst=\""+nw_dst+"\""); }
		if (this.hasNwDstMask())
		{ fields.add("nw_dst_mask="+nw_dst_mask); }
		if (this.hasTpSrc())
		{ fields.add("tp_src="+tp_src); }
		if (this.hasTpDst())
		{ fields.add("tp_dst="+tp_dst); }
		if (this.hasTpSrc() || this.hasTpDst())
		{ fields.add("tp_flip="+tp_flip); }
		if (this.hasDlType())
		{ fields.add("dl_type="+dl_type); }
		if (this.hasNwProto())
		{ fields.add("nw_proto="+nw_proto); }
		
		String result = "{";
		for (String field : fields)
		{ result += field+","; }
		result = result.substring(0, result.length()-1) + "}";
		
		return result;
		
		/*return "{nw_src=\""+nw_src+"\",nw_dst=\""+nw_dst
				+"\",tp_src="+tp_src+",tp_dst="+tp_dst+",tp_flip="+tp_flip
				+",dl_type="+dl_type+",nw_proto="+nw_proto+"}";*/
	}
	
	public List<OFMatch> toOFMatches()
	{
		OFMatch match = new OFMatch();
		int notWildcard = 0;
		int wildcard = OFMatch.OFPFW_ALL;
		
		if (this.hasDlType())
		{
			match.setDataLayerType(this.getDlType());
			notWildcard |= OFMatch.OFPFW_DL_TYPE;
		}
		if (this.hasNwProto())
		{
			match.setNetworkProtocol(this.getNwProto());
			notWildcard |= OFMatch.OFPFW_NW_PROTO;
		}
		if (this.hasNwSrc())
		{
			match.setNetworkSource(IPv4.toIPv4Address(this.getNwSrc()));
			if (this.hasNwSrcMask())
			{ wildcard = (wildcard & ~OFMatch.OFPFW_NW_SRC_MASK) | ((32 - this.getNwSrcMask()) << OFMatch.OFPFW_NW_SRC_SHIFT); }
			else
			{ notWildcard |= OFMatch.OFPFW_NW_SRC_ALL; }
		}
		if (this.hasNwDst())
		{
			match.setNetworkDestination(IPv4.toIPv4Address(this.getNwDst()));
			if (this.hasNwSrcMask())
			{ wildcard = (wildcard & ~OFMatch.OFPFW_NW_DST_MASK) | ((32 - this.getNwDstMask()) << OFMatch.OFPFW_NW_DST_SHIFT); }
			else
			{ notWildcard |= OFMatch.OFPFW_NW_DST_ALL; }
		}
		if (this.hasTpSrc())
		{
			match.setTransportSource(this.getTpSrc());
			notWildcard |= OFMatch.OFPFW_TP_SRC;
		}
		if (this.hasTpDst())
		{
			match.setTransportDestination(this.getTpDst());
			notWildcard |= OFMatch.OFPFW_TP_DST;
		}

		match.setWildcards(wildcard & ~notWildcard);
		
		ArrayList<OFMatch> matches = new ArrayList<OFMatch>();
		matches.add(match);
		if (this.getTpFlip())
		{
			// Go through current matches and add flipped versions if necessary
			ArrayList<OFMatch> newMatches = new ArrayList<OFMatch>();
			for (OFMatch currMatch : matches)
			{				
				// Get tp_src and tp_dst from match
				short tp_src = currMatch.getTransportSource();
				boolean tp_src_wild = ((currMatch.getWildcards() & OFMatch.OFPFW_TP_SRC) != 0);
				short tp_dst = currMatch.getTransportDestination();
				boolean tp_dst_wild = ((currMatch.getWildcards() & OFMatch.OFPFW_TP_DST) != 0);
				
				// If both tp_src and tp_dst are wild, then there is nothing to flip
				if (tp_src_wild && tp_dst_wild)
				{ continue; }
				
				// Create new match with tp_src and tp_dst flipped
				OFMatch newMatch = currMatch.clone();
				newMatch.setTransportSource(tp_dst);
				newMatch.setTransportDestination(tp_src);
				int newWild = newMatch.getWildcards() & ~(OFMatch.OFPFW_TP_SRC | OFMatch.OFPFW_TP_DST);
				if (tp_src_wild)
				{ newWild |= OFMatch.OFPFW_TP_DST; }
				if (tp_dst_wild)
				{ newWild |= OFMatch.OFPFW_TP_SRC; }
				newMatch.setWildcards(newWild);
				
				// Add new match to set of new matches
				newMatches.add(newMatch);
			}
			
			// Add new matches to set of matches
			for(OFMatch newMatch : newMatches)
			{ matches.add(newMatch); }
		}
		
		return matches;
	}
	
	public static PerflowKey fromOFMatch(OFMatch match)
	{
		PerflowKey key = new PerflowKey();
		
		if (0 == (match.getWildcards() & OFMatch.OFPFW_DL_TYPE))
		{ key.setDlType(match.getDataLayerType()); }
		if (0 == (match.getWildcards() & OFMatch.OFPFW_NW_PROTO))
		{ key.setNwProto(match.getNetworkProtocol()); }
		if (match.getNetworkSource() > 0)
		{ key.setNwSrc(IPv4.fromIPv4Address(match.getNetworkSource())); }
		if (match.getNetworkSourceMaskLen() < 32)
		{ key.setNwSrcMask(match.getNetworkSourceMaskLen()); }
		if (match.getNetworkDestination() > 0)
		{ key.setNwDst(IPv4.fromIPv4Address(match.getNetworkDestination())); }
		if (match.getNetworkDestinationMaskLen() < 32)
		{ key.setNwDstMask(match.getNetworkDestinationMaskLen()); }
		if (0 == (match.getWildcards() & OFMatch.OFPFW_TP_SRC))
		{ key.setTpSrc(match.getTransportSource()); }
		if (0 == (match.getWildcards() & OFMatch.OFPFW_TP_DST))
		{ key.setTpDst(match.getTransportDestination()); }
		
		return key;
	}
	
}
