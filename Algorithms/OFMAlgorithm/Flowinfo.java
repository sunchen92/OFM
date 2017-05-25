package OFMAlgorithm;

public class Flowinfo{
	double FlowRate;
	double SLA;
	int flowid;
	boolean MigrateFlag = false;
	
	Flowinfo(double FlowRate, double SLA){
		this.FlowRate = FlowRate;
		this.SLA = SLA;
		this.MigrateFlag = false;
	}
	
	Flowinfo(double FlowRate, double SLA, boolean MigFlag){
		this.FlowRate = FlowRate;
		this.SLA = SLA;
		this.MigrateFlag = MigFlag;
	}
}
