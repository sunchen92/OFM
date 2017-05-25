package edu.wisc.cs.wisdom.sdmbn.apps.testing;

//下面这个import导致运行时找不到module
//import static org.easymock.EasyMock.createMock;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
//import java.util.Iterator;
import java.util.List;
import java.util.Map;
//import java.util.Map.Entry;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.openflow.protocol.OFMatch;
import org.openflow.protocol.OFPort;
//import org.openflow.protocol.OFStatisticsReply;
import org.openflow.protocol.OFStatisticsRequest;
//statistics这是1.2版本有的
//import org.openflow.protocol.statistics.OFAggregateStatisticsRequest;
import org.openflow.protocol.statistics.OFFlowStatisticsReply;
import org.openflow.protocol.statistics.OFFlowStatisticsRequest;
//import org.openflow.protocol.statistics.OFPortStatisticsRequest;
//import org.openflow.protocol.statistics.OFQueueStatisticsRequest;
import org.openflow.protocol.statistics.OFStatistics;
import org.openflow.protocol.statistics.OFStatisticsType;
//import org.openflow.protocol.statistics.OFStatisticsType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.IOFSwitch;
//import net.floodlightcontroller.core.internal.IOFSwitchService;
import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
//import net.floodlightcontroller.statistics.IStatisticsService;
//import net.floodlightcontroller.statistics.StatisticsCollector;
//import net.floodlightcontroller.statistics.SwitchPortBandwidth;
import net.floodlightcontroller.threadpool.IThreadPoolService;
//import sun.net.www.http.KeepAliveCache;
//import net.floodlightcontroller.topology.NodePortTuple;

/**************************************
带宽获取模块
**************************************/
public class MonitorBandwidth implements IFloodlightModule, IMonitorBandwidthService {

	// 日志工具
	private static final Logger log = LoggerFactory.getLogger(MonitorBandwidth.class);
	// Floodlight最核心的service类，其他service类需要该类提供
	protected static IFloodlightProviderService floodlightProvider;
	// //链路数据分析模块，已经由Floodlight实现了，我们只需要调用一下就可以，然后对结果稍做加工，便于我们自己使用
	// protected static IStatisticsService statisticsService;
	// Floodllight实现的线程池，当然我们也可以使用Java自带的，但推荐使用这个
	private static IThreadPoolService threadPoolService;
	// Future类，不明白的可以百度 Java现成future,其实C++11也有这个玩意了
	private static ScheduledFuture<?> portBandwidthCollector;
	// //交换机相关的service,通过这个服务，我们可以获取所有的交换机，即DataPath
	// private static IOFSwitchService switchService;
	// 存放每条链路的带宽使用情况
	private static List<Double> bandwidth = new ArrayList<Double>();
	private static List<Long> byteBefore = new ArrayList<Long>();
	private static List<Long> byteNow = new ArrayList<Long>();
	private static long timeBefore = 0;
	private static long timeNow = 0;
	// 搜集数据的周期
	private static final int portBandwidthInterval = 8;
	private boolean reqCount = false;
	List<OFStatistics> statistics = new ArrayList<OFStatistics>();

	public List<Double> getBandwidth() {
		return bandwidth;
	}

	// 告诉FL，我们添加了一个模块，提供了IMonitorBandwidthService
	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleServices() {
		Collection<Class<? extends IFloodlightService>> l = new ArrayList<Class<? extends IFloodlightService>>();
		l.add(IMonitorBandwidthService.class);
		return l;
	}

	// 我们前面声明了几个需要使用的service,在这里说明一下实现类
	@Override
	public Map<Class<? extends IFloodlightService>, IFloodlightService> getServiceImpls() {
		Map<Class<? extends IFloodlightService>, IFloodlightService> m = new HashMap<Class<? extends IFloodlightService>, IFloodlightService>();
		m.put(IMonitorBandwidthService.class, this);
		return m;
	}

	// 告诉FL我们以来那些服务，以便于加载
	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleDependencies() {
		Collection<Class<? extends IFloodlightService>> l = new ArrayList<Class<? extends IFloodlightService>>();
		l.add(IFloodlightProviderService.class);
		// l.add(IStatisticsService.class);
		// l.add(IOFSwitch.class);
		l.add(IThreadPoolService.class);
		return l;
	}

	// 初始化这些service,个人理解这个要早于startUp()方法的执行，验证很简单，在两个方法里打印当前时间就可以。
	@Override
	public void init(FloodlightModuleContext context) throws FloodlightModuleException {
		floodlightProvider = context.getServiceImpl(IFloodlightProviderService.class);
		// statisticsService = context.getServiceImpl(IStatisticsService.class);
		// switchService = context.getServiceImpl(IOFSwitch.class);
		threadPoolService = context.getServiceImpl(IThreadPoolService.class);
	}

	@Override
	public void startUp(FloodlightModuleContext context) {																										
		startCollectBandwidth();
	}

	// 自定义的开始收集数据的方法，使用了线程池，定周期的执行
	private synchronized void startCollectBandwidth() {
		portBandwidthCollector = threadPoolService.getScheduledExecutor().scheduleAtFixedRate(new GetBandwidthThread(),
				portBandwidthInterval, portBandwidthInterval, TimeUnit.SECONDS);
		log.warn("Statistics collection thread(s) started");
	}

	// 自定义的线程类，在上面的方法中实例化，并被调用
	/**
	 * Single thread for collecting switch statistics and containing the reply.
	 */
	private class GetBandwidthThread extends Thread implements Runnable {
		public List<Double> getBandwidth() {
			return bandwidth;
		}

		@Override
		public void run() {
			// IOFSwitch sw = floodlightProvider.getSwitches().get(switchId);
			Map<Long, IOFSwitch> switches = floodlightProvider.getSwitches();
			if(!(switches.isEmpty())){
				for(IOFSwitch sw : switches.values()){
					log.warn("发送请求信息，获取交换机sw上的FLOW相关信息");
//					 发送请求信息，获取交换机sw上的FLOW相关信息
					Future<List<OFStatistics>> future;
					List<OFStatistics> values = null;
					OFStatisticsRequest req = new OFStatisticsRequest();
					req.setStatisticType(OFStatisticsType.FLOW);
					int requestLength = req.getLengthU();
					OFFlowStatisticsRequest specificReq = new OFFlowStatisticsRequest();
					OFMatch match = new OFMatch();
					match.setWildcards(0xffffffff);
					specificReq.setMatch(match);
					specificReq.setOutPort(OFPort.OFPP_NONE.getValue());
					specificReq.setTableId((byte) 0xff);
					req.setStatistics(Collections.singletonList((OFStatistics) specificReq));
					requestLength += specificReq.getLength();
					req.setLengthU(requestLength);
					try {
						future = sw.getStatistics(req);
						values = future.get(4, TimeUnit.SECONDS);						
					} catch (Exception e) {
						log.error("Failure retrieving statistics from switch " + sw, e);
					}
					log.warn("收到reply消息!");
					
					//analyze reply
					if(values.isEmpty()){
						log.warn("values are empty!");
					}
					else{
						timeNow = System.currentTimeMillis();
//						log.warn("time" + timeNow + "millionseconds!");
						byteNow.clear();
						for (OFStatistics tempOFStat : values) {
							if (tempOFStat instanceof OFFlowStatisticsReply) {
								OFFlowStatisticsReply tempOFFlowStat = (OFFlowStatisticsReply) tempOFStat;
								byteNow.add(Long.valueOf(tempOFFlowStat.getByteCount()));
								log.warn("Now receive  "+ tempOFFlowStat.getByteCount() + "  bytes!");
							}
						}
						log.warn("byteNow size: " + byteNow.size());
						
						//bandwidth
						bandwidth.clear();
						for (int i = 0; i < byteNow.size(); i++) {
							if (reqCount == true) {
								bandwidth.add(Double.valueOf((byteNow.get(i).longValue() - byteBefore.get(i).longValue())*8000/(timeNow - timeBefore)));
								log.warn("bandwidth  " + bandwidth.get(i).doubleValue() + "  bps!");
							}	
						}
						byteBefore.clear();
						for(int i =0; i < byteNow.size(); i++){
							byteBefore.add(Long.valueOf(byteNow.get(i).longValue()));
						}
						reqCount = true;
						timeBefore = timeNow;
						log.warn("bandwidth.size(): " + bandwidth.size());
					}
				}
			}
			else {
				log.warn("NO SWITCH!");
			}
		}
	}
}