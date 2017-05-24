package edu.wisc.cs.wisdom.sdmbn.apps.testing;

import java.util.List;
//import java.util.Map;

//import org.openflow.protocol.statistics.OFStatistics;
//import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.core.module.IFloodlightService;
//import net.floodlightcontroller.topology.NodePortTuple;

public interface IMonitorBandwidthService extends IFloodlightService {
    //带宽使用情况
	public List<Double> getBandwidth();
}