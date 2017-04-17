package edu.wisc.cs.wisdom.sdmbn.apps.nsdi;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;
import net.floodlightcontroller.staticflowentry.IStaticFlowEntryPusherService;

import org.openflow.protocol.OFType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.ISdmbnListener;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnService;
import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.apps.TestbedConstants;
import edu.wisc.cs.wisdom.sdmbn.utils.ChangeForwardingTask;
import edu.wisc.cs.wisdom.sdmbn.utils.FlowUtil;
import edu.wisc.cs.wisdom.sdmbn.utils.ISdmbnApp;
import edu.wisc.cs.wisdom.sdmbn.utils.JoinTask;
import edu.wisc.cs.wisdom.sdmbn.utils.IOperationFinishedTask;
import edu.wisc.cs.wisdom.sdmbn.utils.NextStepTask;
import edu.wisc.cs.wisdom.sdmbn.utils.TraceLoad;

public class FullScenarioNSDI implements IFloodlightModule, ISdmbnListener, ISdmbnApp
{
	private static final short SENDER_PORT = TestbedConstants.PORT_GONDOR;
	private static final String SENDER_IP = "10.0.1.49";
	
	private static final Mode CURRENT_MODE = Mode.OPENNF;
	private static final int TRACE_RATE = 1000;
	
	enum Mode { OPENNF, STRATOS, VENDOR };
	
	/*private static final String CLIENT_SUBNETS[] = { "72.33.0.0", "128.104.0.0", "144.92.0.0" };
	private static final int CLIENT_MASK = 16;
	private static final String SERVER_SUBNETS[] = { "23.1.1.1", "50.2.2.2", "107.3.3.3", "174.4.4.4", "184.5.5.5" };
	private static final int SERVER_MASK = 8;*/
	
	private static final short TRANSPORT_PORTS[] = { 80, 443 }; 
	
	// Service providers
	private IFloodlightProviderService floodlightProvider;
	private IStaticFlowEntryPusherService staticFlowEntryPusher;
	private ISdmbnService sdmbnProvider;
	
	// Active middleboxes
	private Map<String, Middlebox> middleboxes;
	private List<Middlebox> activeIds;
	private List<Middlebox> activeMon;
	
	private int lastActiveIdsIndex = 0;
	
	// Tasks to run when operations finish
	private Map<Integer, IOperationFinishedTask> operationFinishTasks;
	
	private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
	
	private PacketInHandler packetInHandler;
	
	private TraceLoad traceLoad;
	
	protected static Logger log = LoggerFactory.getLogger(FullScenarioNSDI.class);
				
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
		dependencies.add(IFloodlightProviderService.class);
		dependencies.add(IStaticFlowEntryPusherService.class);
		dependencies.add(ISdmbnService.class);
		return dependencies;
	}

	@Override
	public void init(FloodlightModuleContext context)
			throws FloodlightModuleException 
	{
		this.floodlightProvider = context.getServiceImpl(IFloodlightProviderService.class);
		this.staticFlowEntryPusher = context.getServiceImpl(IStaticFlowEntryPusherService.class);	
		this.sdmbnProvider = context.getServiceImpl(ISdmbnService.class);
		
		this.middleboxes = new HashMap<String, Middlebox>();
		this.activeIds = new ArrayList<Middlebox>();
		this.activeMon = new ArrayList<Middlebox>();
		this.operationFinishTasks = new HashMap<Integer,IOperationFinishedTask>();
		
		this.packetInHandler = new PacketInHandler(this);
		
		//-1 is the numpkts argument; it signifies all pkts need to be replayed
		this.traceLoad = new TraceLoad(SENDER_IP, TRACE_RATE, -1);
	}

	@Override
	public void startUp(FloodlightModuleContext context) 
	{
		switch (CURRENT_MODE)
		{
		case STRATOS:
			floodlightProvider.addOFMessageListener(OFType.PACKET_IN, this.packetInHandler); 
			break;
		}
		sdmbnProvider.addSdmbnListener(this);
	}

	@Override
	public void operationFinished(int id) 
	{
		log.info("Operation ({}) finished", id);
		synchronized(this.operationFinishTasks)
		{
			if (this.operationFinishTasks.containsKey(id))
			{
				this.operationFinishTasks.get(id).run(id);
				this.operationFinishTasks.remove(id);
			}
		}
	}
	
	@Override
	public void operationFailed(int id) 
	{ log.error("Operation ({}) failed", id); }
	
	private void addMiddlebox(Middlebox mb)
	{
		if (middleboxes.values().contains(mb))
		{ return; }
		
		if (mb.getId().contains("lyrebird"))
		{ middleboxes.put("ids1", mb); }
		else if (mb.getId().contains("raven"))
		{ middleboxes.put("ids2", mb); }
		else if (mb.getId().contains("sparrow"))
		{ middleboxes.put("mon1", mb); }
		else if (mb.getId().contains("wren"))
		{ middleboxes.put("mon2", mb); }
		
		if (4 == middleboxes.size())
		{ this.executeStep(0); }
	}

	@Override
	public void middleboxConnected(Middlebox mb) 
	{
		if (!middleboxes.keySet().contains(mb))
		{ addMiddlebox(mb);	}
	}
	
	@Override
	public void middleboxDisconnected(Middlebox mb) 
	{ }

	@Override
	public void middleboxLocationUpdated(Middlebox mb) 
	{
		if (!middleboxes.keySet().contains(mb))
		{ addMiddlebox(mb);	}
	}
	
	public void changeForwarding(PerflowKey key, List<Middlebox> mbs)
	{
		Middlebox ids = mbs.get(0);
		Middlebox mon = mbs.get(1);
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		//key.setNwSrc(serverSubnet);
		//key.setNwSrcMask(SERVER_MASK);
		FlowUtil.installForwardingRule("from" + key.getTpSrc() + "toIds", ids.getSwitch(), 
				SENDER_PORT, ids.getSwitchPort(), key, (short)1000, 
				staticFlowEntryPusher);
		FlowUtil.installForwardingRule("from" + key.getTpSrc() + "toMon", mon.getSwitch(), 
				ids.getSwitchPort(), mon.getSwitchPort(), key, (short)1000, 
				staticFlowEntryPusher);
			
		//key.setNwSrc(null);
		//key.setNwDst(serverSubnet);
		//key.setNwDstMask(SERVER_MASK);
		key.setTpDst(key.getTpSrc());
		key.setTpSrc(null);
		FlowUtil.installForwardingRule("to" + key.getTpDst() + "toIds", ids.getSwitch(), 
				SENDER_PORT, ids.getSwitchPort(), key, (short)1000, 
				staticFlowEntryPusher);
		FlowUtil.installForwardingRule("to" + key.getTpDst() + "toMon", mon.getSwitch(), 
				ids.getSwitchPort(), mon.getSwitchPort(), key, (short)1000, 
				staticFlowEntryPusher);
	}
	
	@Override
	public void executeStep(int step)
	{
		int startNextAfter = 0;
		switch(step)
		{
		case 0:
			startNextAfter = 0;//5;
			break;
		case 1:
			this.initialRuleSetup();
			this.traceLoad.startTrace(TestbedConstants.TRACE1);
			startNextAfter = 30;//60;
			break;
		case 2:
			this.traceLoad.startTrace(TestbedConstants.TRACE2);
			startNextAfter = 10;//15;
			break;
		case 3:
			this.scaleUp();
			startNextAfter = 30; //60;
			break;
		case 4:
			this.traceLoad.stopTrace(TestbedConstants.TRACE2);
			startNextAfter = 10;
			break;
		case 5:
			this.scaleDown();
			startNextAfter = 30; //60;
			break;
		case 6:
			this.traceLoad.stopTrace(TestbedConstants.TRACE1);
			startNextAfter = 0;
			break;
		default:
			return;
		}
		this.scheduler.schedule(new NextStepTask(step+1, this), startNextAfter, TimeUnit.SECONDS);
	}
	
	private void initialRuleSetup()
	{
		log.info("Start initial rule setup");
		Middlebox ids1 = middleboxes.get("ids1");
		synchronized(this.activeIds)
		{ this.activeIds.add(ids1); }
		
		Middlebox mon1 = middleboxes.get("mon1");
		synchronized(this.activeMon)
		{ this.activeMon.add(mon1); }
		
		switch(CURRENT_MODE)
		{
		case OPENNF:
		case VENDOR:
			//for (String serverSubnet : SERVER_SUBNETS)
			//{ this.changeForwarding(serverSubnet, ids1); }
			List<Middlebox> mbs = Arrays.asList(new Middlebox[]{ids1,mon1});
			for (short tp_src : TRANSPORT_PORTS)
			{
				PerflowKey key = new PerflowKey();
				key.setTpSrc(tp_src);
				this.changeForwarding(key, mbs);
			}
			break;
		}
	}
	
	private void scaleUp()
	{
		log.info("Start scale up");
		Middlebox ids1 = middleboxes.get("ids1");
		Middlebox ids2 = middleboxes.get("ids2");
		synchronized(this.activeIds)
		{ this.activeIds.add(ids2); }
		
		Middlebox mon1 = middleboxes.get("mon1");
		Middlebox mon2 = middleboxes.get("mon2");
		synchronized(this.activeMon)
		{ this.activeIds.add(mon2); }
		
		switch(CURRENT_MODE)
		{
		case OPENNF:
		case VENDOR:
			//String serverSubnet = SERVER_SUBNETS[1];
			short tp_src = TRANSPORT_PORTS[1];
			
			PerflowKey key = new PerflowKey();
			key.setDlType(Ethernet.TYPE_IPv4);
			key.setNwProto(IPv4.PROTOCOL_TCP);
			key.setTpSrc(tp_src);
			//key.setNwSrc(serverSubnet);
			//key.setNwSrcMask(SERVER_MASK);
			List<Middlebox> mbs = Arrays.asList(new Middlebox[]{ids2,mon2});
			ChangeForwardingTask changeForwardingTask =	
					new ChangeForwardingTask(key, mbs, this);
		
			synchronized(this.operationFinishTasks)
			{
				Collection<Integer> joinOpIds = new ArrayList<Integer>();
				//int idsMoveOpId = sdmbnProvider.moveState(ids1, ids2, key);
				//joinOpIds.add(idsMoveOpId);
				//int monMoveOpId = sdmbnProvider.moveState(mon1, mon2, key);
				//joinOpIds.add(monMoveOpId);
				
				log.info("Initiated moves");
				
				//JoinTask joinTask = new JoinTask(joinOpIds, changeForwardingTask);
				//this.operationFinishTasks.put(idsMoveOpId, joinTask);
				//this.operationFinishTasks.put(monMoveOpId, joinTask);
			}
			break;
		}
	}
	
	private void scaleDown()
	{
		log.info("Start scale down");
		Middlebox ids1 = middleboxes.get("ids1");
		Middlebox ids2 = middleboxes.get("ids2");
		synchronized(this.activeIds)
		{ this.activeIds.remove(ids2); }
		
		Middlebox mon1 = middleboxes.get("mon1");
		Middlebox mon2 = middleboxes.get("mon2");
		synchronized(this.activeMon)
		{ this.activeMon.remove(mon2); }
		
		switch(CURRENT_MODE)
		{
		case OPENNF:
		case VENDOR:
			//String serverSubnet = SERVER_SUBNETS[1];
			short tp_src = TRANSPORT_PORTS[1];
			
			PerflowKey key = new PerflowKey();
			key.setDlType(Ethernet.TYPE_IPv4);
			key.setNwProto(IPv4.PROTOCOL_TCP);
			key.setTpSrc(tp_src);
			//key.setNwSrc(serverSubnet);
			//key.setNwSrcMask(SERVER_MASK);
			List<Middlebox> mbs = Arrays.asList(new Middlebox[]{ids1,mon1});
			ChangeForwardingTask changeForwardingTask =	
					new ChangeForwardingTask(key, mbs, this);
			
			synchronized(this.operationFinishTasks)
			{
				Collection<Integer> joinOpIds = new ArrayList<Integer>();
				//int idsMoveOpId = sdmbnProvider.moveState(ids2, ids1, key);
				//joinOpIds.add(idsMoveOpId);
				//int monMoveOpId = sdmbnProvider.moveState(mon2, mon1, key);
				//joinOpIds.add(monMoveOpId);
				
				log.info("Initiated moves");
				
				//JoinTask joinTask = new JoinTask(joinOpIds, changeForwardingTask);
				//this.operationFinishTasks.put(idsMoveOpId, joinTask);
				//this.operationFinishTasks.put(monMoveOpId, joinTask);
				
				/*int opId = sdmbnProvider.moveState(ids2, ids1, key);
				log.info("[Demo] - initiated move");
				this.operationFinishTasks.put(opId, changeForwardingTask);*/
			}
			break;
		}
	}
	
	public Middlebox getNextActiveIds()
	{
		synchronized(this.activeIds)
		{
			if (this.activeIds.isEmpty())
			{ return null; }
			
			lastActiveIdsIndex++;
			if (lastActiveIdsIndex >= this.activeIds.size())
			{ lastActiveIdsIndex = 0; }
			
			return this.activeIds.get(lastActiveIdsIndex);
		}
	}
}