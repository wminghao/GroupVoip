package org.red5.server.mixer;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class ProcessPipe implements Runnable{
	private boolean bTest = true; //read from a file instead

    public interface Delegate {
        public void onSegmentOutput(byte[] src, int srcLen);
    }
	private Delegate delegate;
	
	public ProcessPipe(Delegate delegate)
	{
		this.delegate = delegate;
	}
	
	public void handleSegInput(ByteBuffer seg)
	{
		if(!bTest) {
			//TODO	
		}
	}

	@Override
	public void run() {
		if(bTest) {
			//read a segment file and send it over
    		final String INPUT_FILE_NAME = "/Users/wminghao/Develop/red5-server/red5-readonly/testvideos/fourflvtest.seg";
    		System.out.println("Reading in binary file named : " + INPUT_FILE_NAME);
    	    File file = new File(INPUT_FILE_NAME);
    	    System.out.println("File size: " + file.length());
    	    byte[] result = new byte[4096];
    	    try {
    	      InputStream input = null;
    	      try {
    	    	int bytesTotal = 0;
    	        input = new BufferedInputStream(new FileInputStream(file));
    	        while(bytesTotal<file.length()) {
    		        int totalBytesRead = 0;
        	        while(totalBytesRead < result.length && (totalBytesRead+bytesTotal)<file.length()){
        	          int bytesRemaining = result.length - totalBytesRead;
        	          //input.read() returns -1, 0, or more :
        	          int bytesRead = input.read(result, totalBytesRead, bytesRemaining); 
        	          if (bytesRead > 0){
        	            totalBytesRead = totalBytesRead + bytesRead;
        	          }
        	        }
        	        this.delegate.onSegmentOutput(result, totalBytesRead);
        	        bytesTotal += totalBytesRead;
        	        Thread.sleep(300); //sleep 300 ms
        	        System.out.println("Total bytes read: " + bytesTotal);
    	        }
    	      }catch (InterruptedException ex) {
    	    	  System.out.println(ex);
    		  }
    	      finally {
    	    	  System.out.println("Closing input stream.");
    	        input.close();
    	      }
    	    }
    	    catch (FileNotFoundException ex) {
    	    	System.out.println("File not found.");
    	    }
    	    catch (IOException ex) {
    	    	System.out.println(ex);
    	    }
    	}
	}	
}
