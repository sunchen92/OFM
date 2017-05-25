package OFMAlgorithm;

import java.util.ArrayList;
import java.util.List;


public class VNFinfo{
//	List<Double> FlowRate;
//	List<Double> TimeRemaining;
	int VNFid;
	List<Flowinfo> flow;
	double LoadPercentage;
	int FlowNum;
//	double TotalBandwidth;
	
	
	VNFinfo(){
//		FlowRate = new ArrayList<Double>();
//		TimeRemaining = new ArrayList<Double>();
		flow = new ArrayList<Flowinfo>();
		LoadPercentage = 0;
//		TotalBandwidth = 0;
	}
	public void Refresh(){
		double TotalBandwidth = 0;
		for (int i = 0; i < flow.size(); i++ ){
			TotalBandwidth += flow.get(i).FlowRate;
		}
		LoadPercentage = TotalBandwidth/AlgorithmConstants.MaxLoad;
		FlowNum = flow.size();
	}
	
	public void Refresh(List<Flowinfo> newflow){
		flow.clear();
		flow.addAll(newflow);
		double TotalBandwidth = 0;
		for (int i = 0; i < flow.size(); i++ ){
			TotalBandwidth += flow.get(i).FlowRate;
		}
		LoadPercentage = TotalBandwidth/AlgorithmConstants.MaxLoad;
	}
}