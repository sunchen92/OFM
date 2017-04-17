package edu.wisc.cs.wisdom.sdmbn;

public interface ISdmbnListener 
{
	public void operationFinished(int id);
	public void operationFailed(int id);
	public void middleboxConnected(Middlebox mb);
	public void middleboxDisconnected(Middlebox mb);
	public void middleboxLocationUpdated(Middlebox mb);
}
