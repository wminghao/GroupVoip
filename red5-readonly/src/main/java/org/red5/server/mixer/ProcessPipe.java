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
import java.util.Vector;

import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.slf4j.Logger;

public class ProcessPipe implements SegmentParser.Delegate{
	private boolean bLoadFromDisc = false; //read from a file instead
	private boolean bSaveToDisc = false; //log input file to a disc
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
	
	//test only
	private String inputFilePath;
	private String outputFilePath;
	private OutputStream outputFile_ = null;
	private DiscReaderThread discReaderThread_ = null; 
	
	//non-test
	private final String MIXCODER_PROCESS_NAME = "/usr/bin/mix_coder";
	private Process process_ = null;
	private DataInputStream in_ = null;
	private DataOutputStream out_ = null;
	private Vector<ByteBuffer> outBuffers_ = new Vector<ByteBuffer>(100); //vector is threadsafe
	private ProcessReaderThread procReader_ = null;
	private ProcessWriterThread procWriter_ = null;
	private Object synchronizeObject_ = new Object();
	
	//Howard: tried 2 use select() to do process IO, but failed, b/c Java does not supports it. 
	//Have to use 2 threads to do select, which is lame.
	//Details, see https://www.ibm.com/developerworks/community/blogs/pmuellr/entry/java_process_management_arghhs?lang=en
    
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

		//start the process pipe here
		if ( !bLoadFromDisc ) {
    		try {
     	    	process_ = Runtime.getRuntime().exec(MIXCODER_PROCESS_NAME);
    	        in_ = new DataInputStream( new BufferedInputStream(process_.getInputStream()));
    	        out_ = new DataOutputStream( new BufferedOutputStream(process_.getOutputStream()) );
        		log.info("Opening process: {}", MIXCODER_PROCESS_NAME);

    			//start the thread here
        		procReader_ = new ProcessReaderThread();
    			Thread thread1 = new Thread(procReader_, "MixerPipeReader");
    			thread1.start();
        		procWriter_ = new ProcessWriterThread();
    			Thread thread2 = new Thread(procWriter_, "MixerPipeWriter");
    			thread2.start();
     	    } catch (IOException ex) {
       			log.info("=====>Process IO other exception:  {}", ex);
     	    } catch (Exception ex) {
       			log.info("=====>Process other exception:  {}", ex);
     	    }
		} else {
			try {
    			//start the thread here
    			discReaderThread_ = new DiscReaderThread();
    			Thread thread = new Thread(discReaderThread_, "DiscReader");
    			thread.start();
			} catch (Exception ex) {
       			log.info("=====>Disc IO other exception:  {}", ex);
     	    }
		}
	    log.info("======>GroupMixer configuration, bSaveToDisc={}, outPath={}, bLoadFromDisc={}, inPath={}.", bSaveToDisc, outputFilePath, bLoadFromDisc, inputFilePath);
	}
	
	public void handleSegInput(ByteBuffer seg)
	{
		if(bSaveToDisc) {
    	    try {
    	    	//log.info("=====>Writing binary file... totalLen={} size={}", totalLen, seg.limit());
    	    	if( outputFile_ == null ) {
        	    	outputFile_ = new BufferedOutputStream(new FileOutputStream(this.outputFilePath));
    	    	}
    	    	if( outputFile_ != null ) {
        	    	//log.info("=====>array totalLen={} size={}", totalLen, seg.array().length);
    	    		outputFile_.write(seg.array(), 0, seg.array().length);
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
			synchronized(synchronizeObject_) {
				outBuffers_.add(seg);
				synchronizeObject_.notify();
			}
		}
	}
	
	private void tryToWrite() {
		boolean isNoErr = true;
		while( isNoErr ) {
    		while( outBuffers_.size() > 0 && isNoErr) {
        		try {
        			ByteBuffer seg = outBuffers_.remove(0);
        			out_.write(seg.array()); //blocking call
        			out_.flush();
        	    }
        	    catch (Exception err) {
        	    	isNoErr = false;
            		log.info("===============Process Writer IO failed {}=======", err);
        	    }
    		}
    		try {
    			synchronized( synchronizeObject_ ) {
    				while (outBuffers_.isEmpty() && isNoErr) {
    					synchronizeObject_.wait(); //wait until something is added to the outBuffers_
    				}
    			}
    		} catch(Exception err) {
    			isNoErr = false;
        		log.info("===============Process Writer wait failed {}=======", err);
    		}
		}
		log.info("=====>Process Writer thread exits.");
	}
	
	private void tryToRead() {
		byte[] inResult = new byte[256];
		int inBytesTotal = 0;
		boolean isEOF = true;
		while( isEOF ) {
    		try {
        		int bytesRead = in_.read(inResult); //blocking call
               	if (bytesRead > 0){
                    segParser_.readData(inResult, bytesRead); //send to segment parser
                    inBytesTotal += bytesRead;
               	} else if (bytesRead < 0){
               		isEOF = false;
            		log.info("===============read error???=======");
               	} else {
            		log.info("===============read 0 bytes???=======");
               	}
        		//log.info("====>curBytesRead {} TotalBytesRead:  {}", bytesRead, inBytesTotal);
    	    } catch (IOException ex) {
      			log.info("=====>Process Reader IO other exception:  {}", ex);
    	    }
		}
		log.info("=====>Process Reader thread exits.");
	}
	
	class DiscReaderThread implements Runnable {
    	@Override
    	public void run() {
    		log.info("Thread is started");
    		assert( bLoadFromDisc );
			//read a segment file and send it over
    		log.info("Reading in binary file named : {}", inputFilePath);
    	    File file = new File(inputFilePath);
    	    log.info("File size: {}", file.length());
    	    try {
    	    	InputStream input = null;
    	    	try {
        	    	int bytesTotal = 0;
        	    	byte[] result = new byte[4096];
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
    	}
	}
	
	class ProcessReaderThread implements Runnable {
		public void run(){
			log.info("Reading in binary from process: {}", MIXCODER_PROCESS_NAME);
			tryToRead();
		}
	}
	class ProcessWriterThread implements Runnable {
		public void run(){
			log.info("Reading in binary from process: {}", MIXCODER_PROCESS_NAME);
			tryToWrite();
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
    	    	if( in_ != null ) {
    	    		in_.close();
    	    	}
    	    	if( out_ != null ) {
    	    		out_.close();
    	    	}
    	    	if( process_ != null ) {
    	    		process_.destroy();
    	    	}
    	    }    
    	    catch (Exception err) {
    	        err.printStackTrace();
    	    }
		}
	}
}
