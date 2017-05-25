package OFMAlgorithm;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import lpsolve.*;


public class Algorithms {
	
	List<MigrateTuple> MigrateInfo = new ArrayList<MigrateTuple>();
	List<VNFinfo> GlobalVNFinfo = new ArrayList<VNFinfo>();
	int VNFNum;
	
	class MigrateTuple{
		int srcVNFid;
		int flowid;
		int dstVNFid;
		
		MigrateTuple(int srcVNFid, int flowid, int dstVNFid){
			this.srcVNFid = srcVNFid;
			this.flowid = flowid;
			this.dstVNFid = dstVNFid;
		}
	}
	
	Algorithms(){
		
	}
	
	protected void MigrateTotal(){
		MigrateInfo.sort(new TupleFlowidComparator());
		for (int i = MigrateInfo.size() - 1; i >= 0; i--){
//			System.out.println("SrcVNFid:"+MigrateInfo.get(i).srcVNFid+" Flowid:"+MigrateInfo.get(i).flowid+" dstVNFid:"+MigrateInfo.get(i).dstVNFid);
			Migrate(MigrateInfo.get(i));
			GlobalVNFinfo.get(MigrateInfo.get(i).srcVNFid).flow.get(MigrateInfo.get(i).flowid).flowid = GlobalVNFinfo.get(MigrateInfo.get(i).dstVNFid).FlowNum;
			GlobalVNFinfo.get(MigrateInfo.get(i).dstVNFid).flow.add(GlobalVNFinfo.get(MigrateInfo.get(i).srcVNFid).flow.get(MigrateInfo.get(i).flowid));
			GlobalVNFinfo.get(MigrateInfo.get(i).dstVNFid).Refresh();
			GlobalVNFinfo.get(MigrateInfo.get(i).srcVNFid).flow.remove(MigrateInfo.get(i).flowid);
			GlobalVNFinfo.get(MigrateInfo.get(i).srcVNFid).Refresh();
		}

	}
	
	protected boolean Migrate(MigrateTuple Tuple){
		//Migrate API
		return true;
	}
	
	protected boolean NewVNFInstance(){
		//NewVNFInstance API
		GlobalVNFinfo.add(new VNFinfo());
		VNFNum++;
		return true;
	}
	
	public double LoadBalancingCheck(){
		//Step 1
		List<Double> LoadBefore = new ArrayList<Double>();
		List<Double> LoadAfter = new ArrayList<Double>();
		
		for (int i = 0; i < GlobalVNFinfo.size(); i++)
			LoadBefore.add(GlobalVNFinfo.get(i).LoadPercentage);
		
		double avgLoad = 0;
		for (int i = 0; i < GlobalVNFinfo.size(); i++)
			avgLoad += GlobalVNFinfo.get(i).LoadPercentage;
		avgLoad = avgLoad / GlobalVNFinfo.size();
		List<VNFinfo> HeavyVNFList = new ArrayList<VNFinfo>();
		List<VNFinfo> LightVNFList = new ArrayList<VNFinfo>();
		for (int i = 0; i < GlobalVNFinfo.size(); i++){
			if (GlobalVNFinfo.get(i).LoadPercentage > avgLoad)
				HeavyVNFList.add(GlobalVNFinfo.get(i));
			else
				LightVNFList.add(GlobalVNFinfo.get(i));
		}
		
//		System.out.println(HeavyVNFList);
//		System.out.println(LightVNFList);
		
		for (int i = 0; i < HeavyVNFList.size(); i++){
			//Step 2
			VNFinfo srcVNF = HeavyVNFList.get(i);
			for (int f = 0; f < srcVNF.flow.size(); f++){
				Flowinfo currentF = srcVNF.flow.get(f);
				boolean SLAFlag = AlgorithmConstants.SFMT <= currentF.SLA - AlgorithmConstants.la_process;
				boolean BufferFlag = (AlgorithmConstants.SFMT+2*AlgorithmConstants.la_process)*currentF.FlowRate <= AlgorithmConstants.BufferSize;
				if (SLAFlag && BufferFlag)
					currentF.MigrateFlag = true;
				else ;
			}
			
			//Step 3
			srcVNF.flow.sort(new FlowRateComparator());	
			for (int f = srcVNF.flow.size() - 1; f >= 0; f--){
				if (!srcVNF.flow.get(f).MigrateFlag)	//unsafe to migrate
					continue;
				else if (srcVNF.LoadPercentage - srcVNF.flow.get(f).FlowRate/AlgorithmConstants.MaxLoad < avgLoad)	//
					continue;
				else ;
				
				MigrateInfo.add(new MigrateTuple(srcVNF.VNFid, f, -1));	//-1 represent the null
				//Late Remove, so we don't remove here.
//				srcVNF.Refresh();
				srcVNF.LoadPercentage -= srcVNF.flow.get(f).FlowRate/AlgorithmConstants.MaxLoad;
			}
		}
		
		//Step 4
		LightVNFList.sort(new VNFLoadComparator());
		MigrateInfo.sort(new TupleFlowRateComparator());
		int end = 0;
		for (int d = LightVNFList.size() - 1; d >= 0; d--){
			double dstLoad = LightVNFList.get(d).LoadPercentage;
//			System.out.println(LightVNFList.get(d).VNFid);
			for (int f = MigrateInfo.size() - 1; f >= 0; f--){
				if (MigrateInfo.get(f).dstVNFid >= 0)
					continue;
				else if (d == 0)
					MigrateInfo.get(f).dstVNFid = d;
				else ;
				if (dstLoad + GlobalVNFinfo.get(MigrateInfo.get(f).srcVNFid).flow.get(MigrateInfo.get(f).flowid).FlowRate/AlgorithmConstants.MaxLoad < avgLoad || f == end){
					MigrateInfo.get(f).dstVNFid = LightVNFList.get(d).VNFid;
//					System.out.print(MigrateInfo.get(f).srcVNFid+"."+MigrateInfo.get(f).flowid+"."+MigrateInfo.get(f).dstVNFid+"|");
					dstLoad += GlobalVNFinfo.get(MigrateInfo.get(f).srcVNFid).flow.get(MigrateInfo.get(f).flowid).FlowRate/AlgorithmConstants.MaxLoad;
//					System.out.println(dstLoad);
					if (f == end)
						end++;
					else ;
					
				}
				else ;
			}
			LightVNFList.get(d).LoadPercentage = dstLoad;
		}
		
		for (int i = 0; i < GlobalVNFinfo.size(); i++)
			LoadAfter.add(GlobalVNFinfo.get(i).LoadPercentage);

		MigrateTotal();
		return variance(LoadBefore)/variance(LoadAfter);
		

	}
	
	public double ScaleInCheck() throws LpSolveException{
		int n = GlobalVNFinfo.size();
		List<Integer> FlowNum = new ArrayList<Integer>();
		for (int i = 0; i < GlobalVNFinfo.size(); i++){
			FlowNum.add(GlobalVNFinfo.get(i).flow.size());
		}
		
		LpSolve problem = LpSolve.makeLp(0, n*n);
		
		double[] coeffobj = new double [n*n+1];
		double[] coeff3 = new double [n*n+1];
		double[] coeff4 = new double [n*n+1];
		double[] coeff5 = new double [n*n+1];
		
		//constr 1
		for (int i = 1; i <= n*n; i++){
			problem.setBinary(i, true);
		}
//		problem.printLp();
		
		//constr 2
		for (int i = 1; i <= n; i++)
			problem.setBounds((i-1)*n+i, 0, 0);
//		problem.printLp();
		
		//constr 3
		for (int i = 1; i <= n; i++){
			for (int s = 0; s <= n*n; s++)
				coeff3[s] = 0;
			for (int s = 1; s <= n; s++){
				coeff3[(i-1)*n+s] = 1;
				coeff3[n*(s-1)+i] = 1;
			}
			problem.addConstraint(coeff3, LpSolve.LE, 1);
		}		
	
//		problem.printLp();
		
		//constr 4
		for (int d = 1; d <= n; d++){
			for (int f = 0; f < Collections.max(FlowNum); f++){
				for (int i = 0; i < coeff4.length; i++)
					coeff4[i] = 0;
				for (int s = 1; s <= n; s++){
					if (f >= GlobalVNFinfo.get(s-1).flow.size())
						continue;
					else ;
					coeff4[(s-1)*n+d] = GlobalVNFinfo.get(s-1).flow.get(f).FlowRate;
					coeff4[(s-1)*n+d] *= r(AlgorithmConstants.SFMT + AlgorithmConstants.la_process - GlobalVNFinfo.get(s-1).flow.get(f).SLA);
				}
				problem.addConstraint(coeff4, LpSolve.LE, AlgorithmConstants.BufferSize/AlgorithmConstants.SFMT);
			}
					
		}
//		problem.printLp();
		
		//constr 5

		for (int d = 1; d <= n; d++){
			for (int i = 0; i < n*n; i++)
				coeff5[i] = 0;
			for (int s = 1; s <= n; s++){
				coeff5[(s-1)*n+d-1] = GlobalVNFinfo.get(s-1).LoadPercentage;
			}
			problem.addConstraint(coeff5, LpSolve.LE, AlgorithmConstants.TH_SAFE-GlobalVNFinfo.get(d-1).LoadPercentage);
		}
//		problem.printLp();
		
		//objective
		for (int s = 1; s <= n; s++){
			//Calculate cost
			double cost = 0;
			for (int f = 0; f < GlobalVNFinfo.get(s-1).FlowNum; f++)
				cost += GlobalVNFinfo.get(s-1).flow.get(f).FlowRate * r(AlgorithmConstants.SFMT + AlgorithmConstants.la_process - GlobalVNFinfo.get(s-1).flow.get(f).SLA);
			
			cost = cost * AlgorithmConstants.SFMT*AlgorithmConstants.penalty_ALPHA;
			
			for (int d = 1; d <= n; d++){
				coeffobj[(s-1)*n+d] = AlgorithmConstants.benefit_VM - cost;
			}
		}
		problem.setObjFn(coeffobj);
		problem.setMaxim();
//		problem.printLp();
		problem.solve();
//		problem.printObjective();
//		problem.printSolution(10);
//		System.out.println(problem.getTotalIter());

		double benefit = problem.getObjective();
//		System.out.println(benefit);
		problem.deleteLp();
		return benefit;
}

	public double ScaleInRandom() {
		List<Double> Costs = new ArrayList<Double>();
		double TotalBenefit = 0;
		for (int i = 0; i < GlobalVNFinfo.size(); i++){

				//Calculate cost
				double cost = 0;
				for (int f = 0; f < GlobalVNFinfo.get(i).FlowNum; f++)
					cost += GlobalVNFinfo.get(i).flow.get(f).FlowRate * r(AlgorithmConstants.SFMT + AlgorithmConstants.la_process - GlobalVNFinfo.get(i).flow.get(f).SLA);
				cost = cost * AlgorithmConstants.SFMT*AlgorithmConstants.penalty_ALPHA;
				Costs.add(AlgorithmConstants.benefit_VM - cost);
				
			
			if (GlobalVNFinfo.get(i).LoadPercentage < AlgorithmConstants.TH_LOW){
				for (int j = i + 1; j < GlobalVNFinfo.size(); j++){
					if (GlobalVNFinfo.get(i).LoadPercentage + GlobalVNFinfo.get(j).LoadPercentage <= AlgorithmConstants.TH_SAFE){
						//Merge i and j;
//						System.out.println(GlobalVNFinfo.get(i).LoadPercentage +"+"+ GlobalVNFinfo.get(j).LoadPercentage);
						GlobalVNFinfo.get(j).LoadPercentage += GlobalVNFinfo.get(i).LoadPercentage;
						TotalBenefit += Costs.get(i);
//						System.out.println("Merge "+i+" to "+j);
						break;
					}
					else ;
				}
			}
		}
		return TotalBenefit;
	}
	
	public double ScaleOutCheck(int ScaleOut_Mode){
		for (int i = 0; i < GlobalVNFinfo.size(); i++){
			if (GlobalVNFinfo.get(i).LoadPercentage > AlgorithmConstants.TH_HIGH){
				NewVNFInstance();
				
				if (ScaleOut_Mode == AlgorithmConstants.ScaleOut_Mode_Greedy)
					return ScaleOut(i);
				else if (ScaleOut_Mode == AlgorithmConstants.ScaleOut_Mode_Best)
					return ScaleOutBest(i);
				else if (ScaleOut_Mode == AlgorithmConstants.ScaleOut_Mode_Random)
					return ScaleOutRandom(i);				
				else
					return -1;//ScaleOutILP(i);
			}
			else {
				return -1;
			}
		}
		return -1;
	}
	
//	public void ScaleOutILP(int VNFid) throws LpSolveException{
//		VNFinfo thisVNFinfo = GlobalVNFinfo.get(VNFid);
//		LpSolve problem = LpSolve.makeLp(0, thisVNFinfo.FlowNum);
//		
//		//constr 0
//		for (int i = 1; i <= thisVNFinfo.FlowNum; i++){
//			problem.setBinary(i, true);
//		}
//		
//		double[] coeffobj = new double [thisVNFinfo.FlowNum+1];
//		double[] coeff1 = new double [thisVNFinfo.FlowNum+1];
//		double[] coeff2 = new double [thisVNFinfo.FlowNum+1];
//		
//		
//		//constr 1
//		for (int i = 1; i <= thisVNFinfo.FlowNum; i++){
//			coeff2[i] = thisVNFinfo.flow.get(i-1).FlowRate;
//		}
//		
//		//constr 2 & 3
//		for (int i = 1; i <= thisVNFinfo.FlowNum; i++)
//			coeff2[i] = thisVNFinfo.flow.get(i-1).FlowRate;
//		
//		problem.addConstraint(coeff2, LpSolve.LE, AlgorithmConstants.TH_SAFE*AlgorithmConstants.MaxLoad);
//		problem.addConstraint(coeff2, LpSolve.GE, (thisVNFinfo.LoadPercentage - AlgorithmConstants.TH_SAFE)*AlgorithmConstants.MaxLoad);
//		
////		problem.printLp();
//		
//		//objective
//		for (int f = 1; f <= thisVNFinfo.FlowNum; f++)
//			coeffobj[f] = AlgorithmConstants.SFMT*AlgorithmConstants.penalty_ALPHA*thisVNFinfo.flow.get(f).FlowRate * r(AlgorithmConstants.SFMT + AlgorithmConstants.la_process - thisVNFinfo.flow.get(f).SLA);
//			
//		problem.setObjFn(coeffobj);
//		problem.setMaxim();
////		problem.printLp();
//		problem.solve();
//		problem.printSolution(1);
//	}
	
	
	public double ScaleOutBest(int VNFid){

		VNFinfo thisVNFinfo = GlobalVNFinfo.get(VNFid);
		List<Integer> Index = new ArrayList<Integer>();
		List<Integer> result = new ArrayList<Integer>();
		double MaxCost = 0x7fffffff;
		double oldLoad = thisVNFinfo.LoadPercentage;
		for (int i = 0; i < thisVNFinfo.FlowNum; i++)
			Index.add(i);
		for (int f = 1; f < 6; f++){
			List<List<Integer>> Result = nchoosek(Index, f);
			for (int i = 0; i < Result.size(); i++){
				double cost = 0;
				double newLoad = 0;
				for (int j = 0; j < f; j++)
					newLoad += thisVNFinfo.flow.get(Result.get(i).get(j)).FlowRate/AlgorithmConstants.MaxLoad;
				boolean OldUnsafe = oldLoad > AlgorithmConstants.TH_SAFE - newLoad;
				boolean NewSafe = newLoad < AlgorithmConstants.TH_SAFE;
				if (!(OldUnsafe && NewSafe))
					continue;
				else ;
				
				double MigTime = AlgorithmConstants.migtime_GAMMA + f*AlgorithmConstants.migtime_ETA;
				boolean BufferSafe = (MigTime+2*AlgorithmConstants.la_process)*newLoad <= AlgorithmConstants.BufferSize;
				if (!BufferSafe)
					continue;
				else ;
				
				for (int j = 0; j < f; j++)
					cost += thisVNFinfo.flow.get(Result.get(i).get(j)).FlowRate * r(AlgorithmConstants.SFMT + AlgorithmConstants.la_process - thisVNFinfo.flow.get(Result.get(i).get(j)).SLA);
				cost = cost * AlgorithmConstants.SFMT*AlgorithmConstants.penalty_ALPHA;
				
				if (cost < MaxCost){
					result = Result.get(i);
					MaxCost = cost;
				}
				else ;
	
			}
		}
//		System.out.println("Best Cost: "+MaxCost);
		for (int i = 0; i < result.size(); i++){
			MigrateInfo.add(new MigrateTuple(VNFid, result.get(i), GlobalVNFinfo.size()-1));
		}
		MigrateTotal();	
		return MaxCost;
	}
	
	public double ScaleOutRandom(int VNFid){
		
		VNFinfo thisVNFinfo = GlobalVNFinfo.get(VNFid);
		double newLoad = 0;
		double oldLoad = 0;
		boolean ScaleOutFlag = false;
		
		oldLoad = thisVNFinfo.LoadPercentage * AlgorithmConstants.MaxLoad;
//		System.out.println(oldLoad);
		ScaleOutFlag = oldLoad >= AlgorithmConstants.MaxLoad*AlgorithmConstants.TH_SAFE;
		int MigCount = 0;
		List <Integer> FlowidList = new ArrayList<Integer>();
		for (int i = 0; i < thisVNFinfo.flow.size(); i++)
			FlowidList.add(i);

		while (ScaleOutFlag){

			int Index = (int) Math.floor(Math.random()*FlowidList.size());
			int FlowIndex = FlowidList.get(Index);
			FlowidList.remove(Index);

			oldLoad -= thisVNFinfo.flow.get(FlowIndex).FlowRate;
			newLoad += thisVNFinfo.flow.get(FlowIndex).FlowRate;
			MigCount++;
			double MigTime = AlgorithmConstants.migtime_GAMMA + MigCount*AlgorithmConstants.migtime_ETA;
			boolean OldUnsafe,NewSafe,BufferSafe;
			NewSafe = newLoad <= AlgorithmConstants.MaxLoad * AlgorithmConstants.TH_SAFE;
			BufferSafe = (MigTime+2*AlgorithmConstants.la_process)*newLoad <= AlgorithmConstants.BufferSize;
//			System.out.print(OldUnsafe);
//			System.out.print(NewSafe);
//			System.out.print(BufferSafe);
//			System.out.println();
			if (NewSafe && BufferSafe)
				MigrateInfo.add(new MigrateTuple(VNFid, FlowIndex, GlobalVNFinfo.size()-1));
			else ;
			OldUnsafe = oldLoad >= AlgorithmConstants.MaxLoad * AlgorithmConstants.TH_SAFE;
			ScaleOutFlag = OldUnsafe && NewSafe && BufferSafe;
		}
		
//		System.out.println(MigrateInfo.size());
		
		double cost = 0;
		for (int j = 0; j < MigrateInfo.size(); j++)
			cost += thisVNFinfo.flow.get(MigrateInfo.get(j).flowid).FlowRate * r(AlgorithmConstants.migtime_GAMMA + AlgorithmConstants.migtime_ETA * MigrateInfo.size() + AlgorithmConstants.la_process - thisVNFinfo.flow.get(MigrateInfo.get(j).flowid).SLA);
		cost = cost * AlgorithmConstants.SFMT*AlgorithmConstants.penalty_ALPHA;
		
//		System.out.println("Random Cost: "+cost);
		MigrateTotal();
		return cost;
	}
	
	public double ScaleOut(int VNFid){

		VNFinfo thisVNFinfo = GlobalVNFinfo.get(VNFid);
		List<Double> Product = new ArrayList<Double>();
		double newLoad = 0;
		double oldLoad = 0;
		boolean ScaleOutFlag = false;
		
		for (int i = 0; i < thisVNFinfo.flow.size(); i++ ){
//			System.out.println(thisVNFinfo.flow.get(i).FlowRate);
			oldLoad += thisVNFinfo.flow.get(i).FlowRate;
			Product.add(thisVNFinfo.flow.get(i).FlowRate*(thisVNFinfo.flow.get(i).SLA - AlgorithmConstants.la_process - AlgorithmConstants.SFMT));
		}
		
//		System.out.println(oldLoad);
		
		ScaleOutFlag = oldLoad >= AlgorithmConstants.MaxLoad*AlgorithmConstants.TH_SAFE;
		int MigCount = 0;

		while (ScaleOutFlag){
			double MaxProduct = 0x80000000;
			int MaxIndex = 0;

			boolean OldUnsafe,NewSafe,BufferSafe;
			for (int i = 0; i < thisVNFinfo.flow.size(); i++ ){
				if (Product.get(i) > MaxProduct || (Product.get(i) == MaxProduct && thisVNFinfo.flow.get(i).FlowRate > thisVNFinfo.flow.get(MaxIndex).FlowRate)){
					MaxIndex = i;
					MaxProduct = Product.get(i);
				}
				else ;
			}
//			System.out.print(MaxIndex);
			Product.set(MaxIndex, (double) 0x80000000);
			oldLoad -= thisVNFinfo.flow.get(MaxIndex).FlowRate;
//			System.out.println(oldLoad);
			newLoad += thisVNFinfo.flow.get(MaxIndex).FlowRate;
			MigCount++;
			double MigTime = AlgorithmConstants.migtime_GAMMA + MigCount*AlgorithmConstants.migtime_ETA;

			NewSafe = newLoad <= AlgorithmConstants.MaxLoad * AlgorithmConstants.TH_SAFE;
			BufferSafe = (MigTime+2*AlgorithmConstants.la_process)*newLoad <= AlgorithmConstants.BufferSize;
//			System.out.println("OldUnsafe: "+OldUnsafe);
//			System.out.println("Newsafe: "+NewSafe);
//			System.out.println("Buffersafe: "+BufferSafe);
			
			if (NewSafe && BufferSafe)
				MigrateInfo.add(new MigrateTuple(VNFid, MaxIndex, GlobalVNFinfo.size()-1));
			else ;
			
			OldUnsafe = oldLoad >= AlgorithmConstants.MaxLoad * AlgorithmConstants.TH_SAFE;
			ScaleOutFlag = OldUnsafe && NewSafe && BufferSafe;

		}
		
		double cost = 0;
		for (int j = 0; j < MigrateInfo.size(); j++)
			cost += thisVNFinfo.flow.get(MigrateInfo.get(j).flowid).FlowRate * r(AlgorithmConstants.migtime_GAMMA + AlgorithmConstants.migtime_ETA * MigrateInfo.size() + AlgorithmConstants.la_process - thisVNFinfo.flow.get(MigrateInfo.get(j).flowid).SLA);
		cost = cost * AlgorithmConstants.SFMT*AlgorithmConstants.penalty_ALPHA;
		
//		System.out.println("Greedy Cost: "+cost);
		MigrateTotal();
		return cost;
	}
	
	public double r(double x){
		if (x >= 0)
			return x;
		else
			return 0;
	}
	
	public static List<List<Integer>> nchoosek(List<Integer> source, int num){
		List<List<Integer>> result = new ArrayList<List<Integer>>();
		if (source.size() == num){
			result.add(source);
		}
		else if (num > 1){
			List<Integer> sourceparam = new ArrayList<Integer>();
			for (int i = 0; i < source.size() - 1; i++)
				sourceparam.add(source.get(i));

			List<List<Integer>> tempresult = nchoosek(sourceparam, num - 1);
			for (int j = 0; j < tempresult.size(); j++){
				tempresult.get(j).add(source.get(source.size()-1));
			}
			result.addAll(tempresult);
			
			tempresult = nchoosek(sourceparam, num);
			result.addAll(tempresult);
		}
		else{
			for (int i = 0; i < source.size(); i++){
				List<Integer> temp = new ArrayList<Integer>();
				temp.add(source.get(i));
				result.add(temp);
			}
		}
		return result;
	}
	 
	public double variance(List<Double> x){
		double a = 0;
		double b = 0;
		for (int i = 0; i < x.size(); i++){
			a += x.get(i) * x.get(i);
			b += x.get(i);
		}
		a = a / x.size();
		b = b / x.size();
		return (a - b * b);
	}

	public class FlowRateComparator implements Comparator <Flowinfo> {
		public int compare (Flowinfo f1, Flowinfo f2){
			if (f1.FlowRate > f2.FlowRate)
				return 1;
			else if (f1.FlowRate < f2.FlowRate)
				return -1;
			else
				return 0;
		}
	}
	
	public class VNFLoadComparator implements Comparator <VNFinfo> {
		public int compare (VNFinfo d1, VNFinfo d2){
			if (d1.LoadPercentage > d2.LoadPercentage)
				return 1;
			else if (d1.LoadPercentage < d2.LoadPercentage)
				return -1;
			else
				return 0;
		}
	}

	public class TupleFlowRateComparator implements Comparator <MigrateTuple> {
		public int compare (MigrateTuple t1, MigrateTuple t2){
			//try-catch
//			if (GlobalVNFinfo.get(t1.srcVNFid).VNFid != t1.srcVNFid)
//				System.err.print("ErrNum: 01");
//			else if (GlobalVNFinfo.get(t1.srcVNFid).flow.get(t1.flowid).flowid != t1.flowid)
//				System.err.print("ErrNum: 02"+t1.flowid);
//			else if (GlobalVNFinfo.get(t2.srcVNFid).VNFid != t2.srcVNFid)
//				System.err.print("ErrNum: 03");
//			else if (GlobalVNFinfo.get(t2.srcVNFid).flow.get(t2.flowid).flowid != t2.flowid)
//				System.err.print("ErrNum: 04");	
//			else ;
			
			
			if (GlobalVNFinfo.get(t1.srcVNFid).flow.get(t1.flowid).FlowRate > GlobalVNFinfo.get(t2.srcVNFid).flow.get(t2.flowid).FlowRate)
				return 1;
			else if (GlobalVNFinfo.get(t1.srcVNFid).flow.get(t1.flowid).FlowRate < GlobalVNFinfo.get(t2.srcVNFid).flow.get(t2.flowid).FlowRate)
				return -1;
			else
				return 0;
		}
	}
	
	public class TupleFlowidComparator implements Comparator <MigrateTuple>{
		public int compare (MigrateTuple t1, MigrateTuple t2){
			if (t1.flowid > t2.flowid)
				return 1;
			else if (t1.flowid < t2.flowid)
				return -1;
			else
				return 0;
		}
	}
}
