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

import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;
import net.floodlightcontroller.staticflowentry.IStaticFlowEntryPusherService;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.ISdmbnListener;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnService;
import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.apps.TestbedConstants;
import edu.wisc.cs.wisdom.sdmbn.utils.ChangeForwardingTask;
import edu.wisc.cs.wisdom.sdmbn.utils.FlowUtil;
import edu.wisc.cs.wisdom.sdmbn.utils.IOperationFinishedTask;
import edu.wisc.cs.wisdom.sdmbn.utils.ISdmbnApp;
import edu.wisc.cs.wisdom.sdmbn.utils.NextStepTask;
import edu.wisc.cs.wisdom.sdmbn.utils.TraceLoad;

public class PRADSMultiflowScenarioNSDI 
	implements IFloodlightModule, ISdmbnListener, ISdmbnApp
{
	private static final short SENDER_PORT = TestbedConstants.PORT_GONDOR;
	private static final String SENDER_IP = "10.0.1.49";
	
	private static final int TRACE_RATE = 1000;
	
	private static final short TRANSPORT_PORTS[] = { 80, 443 }; 
	
	// Service providers
	private IStaticFlowEntryPusherService staticFlowEntryPusher;
	private ISdmbnService sdmbnProvider;
	
	// Active middleboxes
	private Map<String, Middlebox> middleboxes;
		
	// Tasks to run when operations finish
	private Map<Integer, IOperationFinishedTask> operationFinishTasks;
	
	private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
		
	private TraceLoad traceLoad;
	
	protected static Logger log = LoggerFactory.getLogger(PRADSMultiflowScenarioNSDI.class);
				
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
		this.staticFlowEntryPusher = context.getServiceImpl(IStaticFlowEntryPusherService.class);	
		this.sdmbnProvider = context.getServiceImpl(ISdmbnService.class);
		
		this.middleboxes = new HashMap<String, Middlebox>();
		this.operationFinishTasks = new HashMap<Integer,IOperationFinishedTask>();
				
		//-1 is the numpkts argument; it signifies all pkts need to be replayed
		this.traceLoad = new TraceLoad(SENDER_IP, TRACE_RATE, -1);
	}

	@Override
	public void startUp(FloodlightModuleContext context) 
	{
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
		{ middleboxes.put("mon1", mb); }
		else if (mb.getId().contains("raven"))
		{ middleboxes.put("mon2", mb); }
		
		if (2 == middleboxes.size())
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
		Middlebox mon = mbs.get(0);
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		key.setTpSrc(null);
		//key.setTpSrc(tp_src);
		FlowUtil.installForwardingRule("from" + key.getTpSrc() + "toMon", mon.getSwitch(), 
				SENDER_PORT, mon.getSwitchPort(), key, (short)1000, 
				staticFlowEntryPusher);
			
		//key.setTpSrc(null);
		//key.setTpDst(tp_src);
		//FlowUtil.installForwardingRule("to" + tp_src + "toMon", mon.getSwitch(), 
		//		SENDER_PORT, mon.getSwitchPort(), key, (short)1000, 
		//		staticFlowEntryPusher);
	}
	
	public void executeStep(int step)
	{
		Middlebox mon1 = middleboxes.get("mon1");
		int startNextAfter = 0;
		switch(step)
		{
		case 0:
			startNextAfter = 0;
			break;
		case 1:
			this.traceLoad.startTrace(TestbedConstants.TRACE5);
			this.initialRuleSetup();
			startNextAfter = 20;
			break;
		case 2:
			this.cloneMultiflow();
			this.scaleUp();
			startNextAfter = 5; //60;
			break;
		case 3:
			this.traceLoad.stopTrace(TestbedConstants.TRACE5);
			break;
		default:
			return;
		}
		this.scheduler.schedule(new NextStepTask(step+1, this), startNextAfter, TimeUnit.SECONDS);
	}
	
	private void initialRuleSetup()
	{
		log.info("Start initial rule setup");
		
		Middlebox mon1 = middleboxes.get("mon1");
		
		for (short tp_src : TRANSPORT_PORTS)
		{
			PerflowKey key = new PerflowKey();
			key.setTpSrc(tp_src);
			this.changeForwarding(key, Arrays.asList(new Middlebox[]{mon1}));
		}
	}
	
	private void cloneMultiflow()
	{
		log.info("[Demo] - start clone multiflow");	
		Middlebox mon1 = middleboxes.get("mon1");
		Middlebox mon2 = middleboxes.get("mon2");

		synchronized(this.operationFinishTasks)
		{
			//int monMoveOpId = sdmbnProvider.cloneMultiflow(mon1, mon2, null);
			log.info("Initiated clone multiflow");
			//this.operationFinishTasks.put(monMoveOpId, new AfterCloneOperationTask(this));
		}
	}
	
	public void flushMultiflow(Middlebox mb)
	{
		log.info("Start flush multiflow for "+mb.getId());	
		//Middlebox squid1 = middleboxes.get("squid1");
		PerflowKey key = new PerflowKey();
		//PerflowKey key = new MultiflowKey();
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		synchronized(this.operationFinishTasks)
		{
			//int flushOpId = sdmbnProvider.deleteMultiflow(mb, false, null);
			log.info("Initiated flush multiflow");
			//this.operationFinishTasks.put(flushOpId, new AfterCloneOperationTask(this));
		}
	}

	public void flushPerflow(Middlebox mb)
	{
		log.info("Start flush perflow for "+mb.getId());	
		//Middlebox squid1 = middleboxes.get("squid1");
		PerflowKey key = new PerflowKey();
		//PerflowKey key = new MultiflowKey();
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		synchronized(this.operationFinishTasks)
		{
			//int flushOpId = sdmbnProvider.deletePerflow(mb, key);
			log.info("Initiated flush perflow");
			//this.operationFinishTasks.put(flushOpId, new AfterCloneOperationTask(this));
		}
	}	

	public void scaleUp()
	{
		log.info("Start scale up");	
		Middlebox mon1 = middleboxes.get("mon1");
		Middlebox mon2 = middleboxes.get("mon2");
		
		short tp_src = TRANSPORT_PORTS[1];
		
		PerflowKey key = new PerflowKey();
		//PerflowKey key = new MultiflowKey();
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		//key.setTpSrc(tp_src);
		ChangeForwardingTask changeForwardingTask =	new ChangeForwardingTask(key,
				Arrays.asList(new Middlebox[]{mon2}), this);
	
		synchronized(this.operationFinishTasks)
		{
			//int monMoveOpId = sdmbnProvider.moveState(mon1, mon2, null);
			log.info("Initiated move");
			//this.operationFinishTasks.put(monMoveOpId, changeForwardingTask);
		}
	}
	
	private void scaleDown()
	{
		log.info("Start scale down");
		
		Middlebox mon1 = middleboxes.get("mon1");
		Middlebox mon2 = middleboxes.get("mon2");
		
		short tp_src = TRANSPORT_PORTS[1];
		
		PerflowKey key = new PerflowKey();
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		//key.setTpSrc(tp_src);
		ChangeForwardingTask changeForwardingTask = new ChangeForwardingTask(key, 
						Arrays.asList(new Middlebox[]{mon1}), this);
		
		synchronized(this.operationFinishTasks)
		{
			//int monMoveOpId = sdmbnProvider.moveState(mon2, mon1, key);
			log.info("Initiated move");
			//this.operationFinishTasks.put(monMoveOpId, changeForwardingTask);
		}
	}
}

class AfterCloneOperationTask implements IOperationFinishedTask
{
	PRADSMultiflowScenarioNSDI app;
	
	public AfterCloneOperationTask(PRADSMultiflowScenarioNSDI app)
	{
		this.app = app;
	}
	
	@Override
	public void run(int opId)
	{ this.app.scaleUp(); }
}
