package edu.wisc.cs.wisdom.sdmbn.apps.testing;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;

import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;
import net.floodlightcontroller.staticflowentry.IStaticFlowEntryPusherService;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.Parameters.*;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnListener;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnService;
import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class TestShareStrong implements IFloodlightModule, ISdmbnListener
{
	private boolean initiated = false;
	private short discovered = 0;
	
	// Service providers
	private IStaticFlowEntryPusherService staticFlowEntryPusher;
	private ISdmbnService sdmbnProvider;
	
	//private Middlebox ids1, ids2;
	List<Middlebox> instances = new ArrayList<Middlebox>();
	private PerflowKey key;
	private int opId;
	
	protected static Logger log = LoggerFactory.getLogger(TestShareStrong.class);
				
	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleServices() 
	{ return null; }

	@Override
	public Map<Class<? extends IFloodlightService>, IFloodlightService> getServiceImpls() 
	{ return null; }

	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleDependencies() 
	{
		Collection<Class<? extends IFloodlightService >> dependencies = 
			new ArrayList<Class<? extends IFloodlightService>>();
		dependencies.add(IStaticFlowEntryPusherService.class);
		dependencies.add(ISdmbnService.class);
		return dependencies;
	}

	@Override
	public void init(FloodlightModuleContext context)
			throws FloodlightModuleException 
	{
		staticFlowEntryPusher = context.getServiceImpl(IStaticFlowEntryPusherService.class);	
		sdmbnProvider = context.getServiceImpl(ISdmbnService.class);
		
		key = new PerflowKey();
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		
		opId = -1;
	}

	@Override
	public void startUp(FloodlightModuleContext context) 
	{
		sdmbnProvider.addSdmbnListener(this);
	}

	@Override
	public void operationFinished(int id) 
	{
		if (id != opId)
		{ log.info("Unknown operation ({}) finished", opId); }
	}
	
	@Override
	public void operationFailed(int id) 
	{ log.error("Operation ({}) failed", opId); }

	@Override
	public void middleboxConnected(Middlebox mb) 
	{
		if (instances.contains(mb) == false)
		{
			instances.add(mb);
		}
		initiateForwardingSetup();
		
	}
	
	@Override
	public void middleboxDisconnected(Middlebox mb) 
	{
		instances.remove(mb);
	}

	@Override
	public void middleboxLocationUpdated(Middlebox mb) 
	{
		discovered++;
	}
	
	private  void initiateForwardingSetup()
	{
		log.info("instances.size = "+ instances.size());
		if (instances.size() >= 2 && initiated == false && discovered >=2)
		{
			log.info("setup fwding rules");
			initiated = true;
			initiateShare();
		}
	}
	
	private void initiateShare()
	{
		log.info("[SDMBN] - Initiating share");
		opId = sdmbnProvider.share(instances, key, Scope.MULTIFLOW, Consistency.STRONG);
		log.info("[SDMBN] - returned from initiate share");
	}
}
