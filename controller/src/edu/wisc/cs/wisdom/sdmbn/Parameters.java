package edu.wisc.cs.wisdom.sdmbn;

public class Parameters {
	public enum Scope
	{
		PERFLOW, MULTIFLOW, ALLFLOWS,
		PF_MF, PF_AF, MF_AF, PF_MF_AF
	}
	
	public enum Guarantee
	{
		NO_GUARANTEE, LOSS_FREE, ORDER_PRESERVING
	}

	public enum Consistency
	{
		STRONG, STRICT
	}
	
	public enum Optimization
	{
		//PZ = Parallelize
		//LL = Late Locking
		//ER = Early Release
		NO_OPTIMIZATION, PZ, LL, 
		PZ_LL, PZ_LL_ER
	}
}
