package edu.wisc.cs.wisdom.sdmbn.utils;

import java.util.ArrayList;
import java.util.List;

import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.staticflowentry.IStaticFlowEntryPusherService;

import org.openflow.protocol.OFFlowMod;
import org.openflow.protocol.OFMatch;
import org.openflow.protocol.OFPacketOut;
import org.openflow.protocol.action.OFAction;
import org.openflow.protocol.action.OFActionOutput;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class FlowUtil 
{
	protected static Logger log = LoggerFactory.getLogger(FlowUtil.class);
	
	private static final short DEFAULT_PRIORITY = 1000;
	private static final short TIMEOUT_PERMANENT = 0;
	
	@Deprecated
	public static void installForwardingRule(String name, IOFSwitch sw,
			short inPort, short outPort, PerflowKey key, short priority, 
			IStaticFlowEntryPusherService staticFlowEntryPusher)
	{
		short[] outPorts = new short[1];
		outPorts[0] = outPort;
		FlowUtil.installForwardingRule(name, sw, inPort, outPorts, key, priority, 
				staticFlowEntryPusher);
	}
	
	@Deprecated
	public static void installForwardingRule(String name, IOFSwitch sw,
			short inPort, short[] outPorts, PerflowKey key, short priority, 
			IStaticFlowEntryPusherService staticFlowEntryPusher)
	{
		installForwardingRules(sw, inPort, outPorts, key);	
	}
	
	public static void installForwardingRules(IOFSwitch sw, short inPort, 
			short outPort, PerflowKey key)
	{
		short[] outPorts = new short[1];
		outPorts[0] = outPort;
		FlowUtil.installForwardingRules(sw, inPort, outPorts, key);
		
	}
	
	public static void installForwardingRules(IOFSwitch sw, short inPort, 
			short[] outPorts, PerflowKey key)
	{
		if (null == sw || null == key)
		{
			log.error("Missing arguments: sw="+sw+", key="+key);
			return;
		}
		
		// Prepare flow add message
		OFFlowMod rule = new OFFlowMod();
		rule.setCommand(OFFlowMod.OFPFC_ADD);
		rule.setPriority(DEFAULT_PRIORITY);
		rule.setBufferId(OFPacketOut.BUFFER_ID_NONE);
		rule.setHardTimeout(TIMEOUT_PERMANENT);
		rule.setIdleTimeout(TIMEOUT_PERMANENT);
		
		// Create the actions
		ArrayList<OFAction> actions = new ArrayList<OFAction>();
		for(int i = 0; i < outPorts.length; i++)
		{
			OFAction outputTo = new OFActionOutput(outPorts[i]);
			actions.add(outputTo);
		}
		rule.setActions(actions);
		rule.setLength((short) (OFFlowMod.MINIMUM_LENGTH + 
				(OFActionOutput.MINIMUM_LENGTH * actions.size())));
		
		// Get the matches
		List<OFMatch> matches = key.toOFMatches();
		
		// Create a rule for each match
		for (OFMatch match : matches)
		{
			// Create the match criteria
			match.setInputPort(inPort);
			match.setWildcards(match.getWildcards() & ~OFMatch.OFPFW_IN_PORT);
			rule.setMatch(match);
	
			// Install the rule			
			try
			{
				sw.write(rule, null);
				sw.flush();
			} 
			catch (Exception e)
			{ e.printStackTrace(); }
			
			log.debug("Installed rule: "+rule);
		}
	}
}
