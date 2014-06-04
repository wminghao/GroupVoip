package org.red5.server.mixer;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.slf4j.Logger;

public class ProcessPipe implements Runnable, SegmentParser.Delegate{
	private boolean bLoadFromDisc = false; //read from a file instead
	private boolean bSaveToDisc = false; //log input file to a disc
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
	private boolean bIsPipeStarted = false;
	
	//test only
	private String inputFilePath;
	private String outputFilePath;
	private OutputStream outputFile_ = null;
	
	//non-test
	private final String MIXCODER_PROCESS_NAME = "/usr/bin/mixcoder";
	private DataInputStream in_ = null;
	private DataOutputStream out_ = null;
	
	/*
	 * flv output segment parser
	 */
	private SegmentParser segParser_ = new SegmentParser(this);
	
	private SegmentParser.Delegate delegate;
	
	public ProcessPipe(SegmentParser.Delegate delegate, boolean bSaveToDisc, String outputFilePath, boolean bLoadFromDisc, String inputFilePath)
	{
		this.delegate = delegate;
		this.bSaveToDisc = bSaveToDisc;
		this.outputFilePath = outputFilePath;
		this.bLoadFromDisc = bLoadFromDisc;
		this.inputFilePath = inputFilePath;
	    log.info("======>GroupMixer configuration, bSaveToDisc={}, outPath= {}, bLoadFromDisc={}, inPath={}.", bSaveToDisc, outputFilePath, bLoadFromDisc, inputFilePath);
	}
	
	public void handleSegInput(ByteBuffer seg, int totalLen)
	{
		//start the thread here
    	if( !bIsPipeStarted ) {
    		bIsPipeStarted = true;
    		Thread thread = new Thread(this, "MixerPipe");
    		thread.start();
    	}

		if(bSaveToDisc) {
    	    try {
    	    	//log.info("=====>Writing binary file... totalLen={} size={}", totalLen, seg.limit());
    	    	if( outputFile_ == null ) {
        	    	outputFile_ = new BufferedOutputStream(new FileOutputStream(this.outputFilePath));
    	    	}
    	    	if( outputFile_ != null ) {
        	    	//log.info("=====>array totalLen={} size={}", totalLen, seg.array().length);
    	    		outputFile_.write(seg.array(), 0, totalLen);
    	    	}
    	    }
    	    catch(FileNotFoundException ex){
    		    log.info("======>Output File not found.");
    	    }
    	    catch(IOException ex){
    		    log.info("======>IO exception.");
    	    }
		} 
		if ( !bLoadFromDisc ) {
    		try {
    			if ( in_==null || out_==null ) {
    		        Process p = Runtime.getRuntime().exec(MIXCODER_PROCESS_NAME);
    
    		        in_ = new DataInputStream( p.getInputStream() );
    		        out_ = new DataOutputStream( p.getOutputStream() );
    	    		log.info("Opening process: {}", MIXCODER_PROCESS_NAME);
    			}
    			out_.write(seg.array(), 0, totalLen);
    	    }
    	    catch (Exception err) {
    	        err.printStackTrace();
    	    }
		}
	}

	@Override
	public void run() {
		log.info("Thread is started");
		if( bLoadFromDisc ) {
			//read a segment file and send it over
    		log.info("Reading in binary file named : {}", inputFilePath);
    	    File file = new File(inputFilePath);
    	    log.info("File size: {}", file.length());
    	    byte[] result = new byte[4096];
    	    try {
    	    	InputStream input = null;
    	    	try {
        	    	int bytesTotal = 0;
        	        input = new BufferedInputStream(new FileInputStream(file));
        	        int fileLen = (int) file.length();
        	        while( bytesTotal < fileLen ) {
        		        int bytesToRead = 0;
            	        while(bytesToRead < result.length && (bytesToRead+bytesTotal)<fileLen){
            	        	int bytesRemaining = result.length - bytesToRead;
            	        	//input.read() returns -1, 0, or more :
            	        	int bytesRead = input.read(result, bytesToRead, bytesRemaining); 
            	        	if (bytesRead > 0){
            	        		bytesToRead += bytesRead;
            	        	}
            	        }
            	        segParser_.readData(result, bytesToRead); //send to segment parser
            	        bytesTotal += bytesToRead;
            	        Thread.sleep(10);
    
                		log.info("Total bytes read:  {}, len {}", bytesTotal, fileLen);
        	        }
        	    } catch (InterruptedException ex) {
              		log.info("InterruptedException:  {}", ex);
        	    } finally {
        	     	log.info("Closing input stream.");
        	   	  	input.close();
        	    }
    	    }
    	    catch (FileNotFoundException ex) {
      			log.info("File not found:  {}", ex);
    	    }
    	    catch (IOException ex) {
      			log.info("Other exception:  {}", ex);
    	    }
    	} else {
    		log.info("Reading in binary from process: {}", MIXCODER_PROCESS_NAME);
    		final int BUFFER_MAX_SIZE = 4096;
    	    byte[] result = new byte[BUFFER_MAX_SIZE];
    	    try {
    	    	int bytesTotal = 0;
    	    	boolean bShouldContinue = true;
    	        while( bShouldContinue ) { //TODO wait until pipe is down
        	        //input.read() returns -1, 0, or more :
        	       	int bytesRead = in_.read(result, 0, BUFFER_MAX_SIZE); 
        	       	if (bytesRead > 0){
                        segParser_.readData(result, bytesRead); //send to segment parser
            	        bytesTotal += bytesRead;
        	       	} else if (bytesRead < 0){
        	       		bShouldContinue = false;
        	       	} else {
                		log.info("===============read 0 bytes???=======");
        	       	}
            		log.info("Total bytes read:  {}", bytesTotal);
    	        }
    	    } catch (IOException ex) {
      			log.info("=====>Process IO other exception:  {}", ex);
    	    }
    	}
	}

	@Override
	public void onFrameParsed(int mixerId, ByteBuffer frame, int len) {
		this.delegate.onFrameParsed(mixerId, frame, len);		
	}	
	
	public void close()
	{
		if( bSaveToDisc ) {
    	    try {
    	    	outputFile_.close();
    	    }catch (IOException ex) {
      			log.info("close exception:  {}", ex);
    	    }
		}

		if ( !bLoadFromDisc ) {
    	    try {
    	        in_.close();
    	        out_.close();
    	    }    
    	    catch (Exception err) {
    	        err.printStackTrace();
    	    }
		}
	}
}
