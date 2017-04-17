package edu.wisc.cs.wisdom.sdmbn.operation;

import java.util.ArrayList;

/** describes information returned from MB */
public class SdmbnState {

	// type of answer - GET/EVENT
	private String type = null;
	private ArrayList<String> content = new ArrayList<String>();
	
	public SdmbnState(String type) {
		this.type = type;
	}
	
	public SdmbnState(String type, ArrayList<String> content) {
		this.type   = type;
		this.content.addAll(content);
	}
	
	// manipulation methods
	public String getType() {
		return this.type;
	}
	public void setType(String type) {
		this.type = type;
	}

	public void setContent(ArrayList<String> content) {
		this.content.addAll(content);
	}
	public void addContent(String content) {
		this.content.add(content);
	}
	
	public ArrayList<String> getContent() {
		return this.content;
	}
}

