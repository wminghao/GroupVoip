package org.red5.server.mixer;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.BufferedInputStream;

public class SegmentParserTest implements SegmentParser.Delegate{

	SegmentParser parser;
	public SegmentParserTest() {
		parser = new SegmentParser(this);
	}
	public void readData(byte[] src, int srcLen) {
		parser.readData(src, srcLen);
	}
	
	public static void main(String[] args) {
		SegmentParserTest test = new SegmentParserTest();
		/** Change these settings before running this class. */
		//final String INPUT_FILE_NAME = "../../../../../../../testvideos/singleflvtest.seg";
		final String INPUT_FILE_NAME = "../../../../../../../testvideos/fourflvtest.seg";
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
    	        test.readData(result, totalBytesRead);
    	        bytesTotal += totalBytesRead;
    	        System.out.println("Total bytes read: " + bytesTotal);
	        }
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

	@Override
	public void onFrameParsed(int mixerId, byte[] frame, int len) {
        System.out.println("Read a frame, mixerId: " + mixerId+" len="+len);
	}

}
