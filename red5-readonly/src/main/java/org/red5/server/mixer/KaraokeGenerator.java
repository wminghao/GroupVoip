package org.red5.server.mixer;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.LinkedList;

import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.slf4j.Logger;

public class KaraokeGenerator implements Runnable, FLVParser.Delegate {
	private Delegate delegate_;
	private String karaokeFilePath_;
	private FLVParser flvParser_ = new FLVParser(this);
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
    private LinkedList<FLVFrameObject> flvFrameQueue_ = new LinkedList<FLVFrameObject>();
    private long firstPTS_ = 0xffffffff;
    private boolean bStarted_ = false;
    
    private class FLVFrameObject
    {
    	public ByteBuffer frame;
    	public int len;
    	public int timestamp;
    	public FLVFrameObject(ByteBuffer frame, int len, int timestamp) {
    		this.frame = frame;
    		this.len = len;
    		this.timestamp = timestamp;
    	}
    }
	
    public interface Delegate {
        public void onKaraokeFrameParsed(ByteBuffer frame, int len);
    }

	public KaraokeGenerator(KaraokeGenerator.Delegate delegate, String karaokeFilePath){
		this.delegate_ = delegate;
		this.karaokeFilePath_ = karaokeFilePath;
	}
	
	public void tryToStart() {
		if( !bStarted_ ) {
			bStarted_ = true;
			Thread thread = new Thread(this, "KaraokeThread");
			thread.start();
		}
	}
		
	@Override
	public void run() {
		log.info("Karaoke thread is started");
    	//read a segment file and send it over
    	log.info("Reading in karaoke file named : {}", karaokeFilePath_);
        File file = new File(karaokeFilePath_);
        log.info("File size: {}", file.length());
        try {
        	InputStream input = null;
        	try {
    	    	int bytesTotal = 0;
    	    	byte[] result = new byte[4096];
    	        input = new BufferedInputStream(new FileInputStream(file));
    	        int fileLen = (int) file.length();
    	        //skip 13 bytes header
    	        readBuf(result, input, 13, fileLen);
    	        long startTime = System.currentTimeMillis();
        		log.info("---->Start timestamp:  {}", startTime);
    	        //read frame by frame
    	        while( bytesTotal < fileLen ) {
    	        	if( flvFrameQueue_.size() > 0) {
        	        	while ( true ) {
            	        	FLVFrameObject curFrame = flvFrameQueue_.peek();
        	        		if((curFrame.timestamp - firstPTS_) > ( System.currentTimeMillis() - startTime) ) {
        	        			Thread.sleep( 1 );
        	        		} else {
        	        			break;
        	        		}
        	        	}
    	        		FLVFrameObject curFrame = flvFrameQueue_.remove();
	        			delegate_.onKaraokeFrameParsed(curFrame.frame, curFrame.len);
	            		log.info("---->Popped a frame timestamp:  {}, len {}", curFrame.timestamp, curFrame.len);
	        		}

    	        	if( flvFrameQueue_.size() < 10) {
    	        		//read data
    	        		int bytesRead = readBuf(result, input, bytesTotal, fileLen);
    	        		flvParser_.readData(result, bytesRead); //send to segment parser
    	        		bytesTotal += bytesRead;
                		log.info("Total bytes read:  {}, len {}", bytesTotal, fileLen);
            	        Thread.sleep(1);
    	        	} else {
            	        Thread.sleep(10);
    	        	}
    	        }
    	    } catch (InterruptedException ex) {
          		log.info("InterruptedException:  {}", ex);
    	    } catch (Exception ex) {
          		log.info("General exception:  {}", ex);
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

	@Override
	public void onKaraokeFrameParsed(ByteBuffer frame, int len, int timestamp) {
		if( firstPTS_ == 0xffffffff ) {
			//send the first frame immediately
			firstPTS_ = timestamp;
			delegate_.onKaraokeFrameParsed(frame, len);
    		log.info("---->First frame timestamp: {} len: {}", firstPTS_, len);
		} else {
			//for the rest, put into the queue first
			flvFrameQueue_.add( new FLVFrameObject(frame, len, timestamp) );
		}
	}
	
	private int readBuf(byte[] result, InputStream input, int bytesTotal, int fileLen) throws IOException {
		int bytesToRead = 0;
        while(bytesToRead < result.length && (bytesToRead+bytesTotal)<fileLen){
        	int bytesRemaining = result.length - bytesToRead;
        	//input.read() returns -1, 0, or more :
        	int bytesRead = input.read(result, bytesToRead, bytesRemaining); 
        	if (bytesRead > 0){
        		bytesToRead += bytesRead;
        	}
        }
        return bytesToRead;
	}
}
