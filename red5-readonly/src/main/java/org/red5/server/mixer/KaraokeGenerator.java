package org.red5.server.mixer;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicBoolean;

import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.slf4j.Logger;

public class KaraokeGenerator implements Runnable, FLVParser.Delegate {
	private Delegate delegate_;
	private String karaokeFilePath_;
	private FLVParser flvParser_ = null;
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
    private LinkedList<FLVFrameObject> flvFrameQueue_ = new LinkedList<FLVFrameObject>();
    private LinkedList<DelayObject> delayObjectQueue_ = new LinkedList<DelayObject>();
    private final static long DELAY_INTERVAL = 100; //delay for 100 milliseconds.
    private int firstPTS_ = 0xffffffff;
    private int lastTimestamp_ = 0;
    private boolean bStarted_ = false;
    
    //key is fileName, value is song name
    Map<String,String> songMappingTable_ = new HashMap<String, String>();
    String curSong_ = "";
    private AtomicBoolean bCancelCurrentSong = new AtomicBoolean(false);;
    
    private class DelayObject 
    {
    	public FLVFrameObject flvFrameObj_;
    	public long enqueTime_;
    	public DelayObject(FLVFrameObject flvFrame) {
    		this.flvFrameObj_ = flvFrame;
    		this.enqueTime_ = System.currentTimeMillis();
    	}    	
    }
    
    private class FLVFrameObject
    {
    	public ByteBuffer frame;
    	public int timestamp;
    	public int length;
    	public FLVFrameObject(ByteBuffer frame, int len, int timestamp) {
    		this.frame = ByteBuffer.allocate(len);
    		this.frame.put(frame.array(), 0, len);
    		this.frame.flip();
    		this.timestamp = timestamp;
    		this.length = len;
    	}
    }
	
    public interface Delegate {
        public void onKaraokeFrameParsed(ByteBuffer frame, int len, boolean bIsDelayed);
    }

	public KaraokeGenerator(KaraokeGenerator.Delegate delegate, String karaokeFilePath){
		this.delegate_ = delegate;
		this.karaokeFilePath_ = karaokeFilePath;
		readSongMappingTable();
	}
	
	public void tryToStart() {
		if( !bStarted_ ) {
			bStarted_ = true;
			Thread thread = new Thread(this, "KaraokeThread");
			thread.start();
		}
	}
	
	private void loadASong(String fileName) {
        firstPTS_ = 0xffffffff;
		File file = new File(fileName);
        log.info("File {} size: {}", fileName, file.length());
        long startTime = System.currentTimeMillis();
        try {
        	flvParser_ = new FLVParser(this, lastTimestamp_);
        	InputStream input = null;
        	try {
    	    	int bytesTotal = 0;
    	    	byte[] result = new byte[4096];
    	        input = new BufferedInputStream(new FileInputStream(file));
    	        int fileLen = (int) file.length() - 13;
    	        //skip 13 bytes header
    	        byte[] header = new byte[13];
    	        input.read(header);
        		//log.info("---->Start timestamp:  {}", startTime);
    	        //read frame by frame
    	        while( (bytesTotal < fileLen || flvFrameQueue_.size() > 0 ) && !bCancelCurrentSong.get()) {
    	        	if( flvFrameQueue_.size() > 0) {
        	        	while ( true ) {
            	        	FLVFrameObject curFrame = flvFrameQueue_.peek();
        	        		if((curFrame.timestamp - firstPTS_) > ( System.currentTimeMillis() - startTime) ) {
        	        			checkForDelaySend();
        	        			Thread.sleep( 1 );
        	        		} else {
        	        			break;
        	        		}
        	        	}
    	        		FLVFrameObject curFrame = flvFrameQueue_.remove();
	        			delegate_.onKaraokeFrameParsed(curFrame.frame, curFrame.frame.capacity(), false);
	        			delayObjectQueue_.add(new DelayObject(curFrame)); //add to the delayed object queue
	            		//log.info("---->Popped a frame timestamp:  {}, len {}", curFrame.timestamp, curFrame.frame.capacity());
	        		}

    	        	if( flvFrameQueue_.size() < 10) {
    	        		//read data
    	        		int bytesRead = readBuf(result, input, bytesTotal, fileLen);
    	        		flvParser_.readData(result, bytesRead); //send to segment parser
    	        		bytesTotal += bytesRead;
                		//log.info("Total bytes read:  {}, len {}", bytesTotal, fileLen);
            	        Thread.sleep(1);
    	        	} else {
            	        Thread.sleep(10);
    	        	}
        			checkForDelaySend();
    	        }
    	    } catch (InterruptedException ex) {
          		log.info("InterruptedException:  {}", ex);
    	    } catch (Exception ex) {
          		log.info("General exception:  {}", ex);
    	    } finally {
    	     	log.info("Closing input stream.");
    	   	  	input.close();
    	    }
        	//empty the flvFrameQueue
        	if(bCancelCurrentSong.get()) {
    	     	log.info("flvFrameQueue_.size()={}", flvFrameQueue_.size());
    	     	while ( flvFrameQueue_.size() > 0 ) {
    	        	FLVFrameObject curFrame = flvFrameQueue_.peek();
	        		if((curFrame.timestamp - firstPTS_) > ( System.currentTimeMillis() - startTime) ) {
	        			Thread.sleep( 1 );
	        		} else {
	            		FLVFrameObject frame = flvFrameQueue_.remove();
	        			delegate_.onKaraokeFrameParsed(frame.frame, frame.frame.capacity(), false);
	        			delayObjectQueue_.add(new DelayObject(frame)); //add to the delayed object queue
	            		//log.info("---->Popped a frame timestamp:  {}, len {}", curFrame.timestamp, curFrame.frame.capacity());
	        		}
        			checkForDelaySend();
	        	}
        	}
        	
        	long duration = emptyDelayObjectQueue();
            lastTimestamp_ += duration; //advance a little bit
			bCancelCurrentSong.compareAndSet(true, false); //set it back to false;
        }
        catch (FileNotFoundException ex) {
    		log.info("File not found:  {}", ex);
        }
        catch (IOException ex) {
    		log.info("Other exception:  {}", ex);
        }
        catch (Exception ex) {
      		log.info("General exception:  {}", ex);
	    } 
	}
		
	@Override
	public void run() {
		log.info("Karaoke thread is started");
    	//read a segment file and send it over
    	log.info("Reading in karaoke filePath: {}", karaokeFilePath_);
    	while(true) {
        	loadASong(curSong_);
    	}
	}

	@Override
	public void onKaraokeFrameParsed(ByteBuffer frame, int len, int timestamp) {
		if( firstPTS_ == 0xffffffff ) {
			//send the first frame immediately
			firstPTS_ = timestamp;
			delegate_.onKaraokeFrameParsed(frame, len, false);
			delayObjectQueue_.add(new DelayObject(new FLVFrameObject(frame, len, timestamp)));
    		//log.info("---->First frame timestamp: {} len: {}", firstPTS_, len);
		} else {
			//for the rest, put into the queue first
			flvFrameQueue_.add( new FLVFrameObject(frame, len, timestamp) );
		}
		lastTimestamp_ = timestamp;
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
	
	private void checkForDelaySend() {
        long curTime = System.currentTimeMillis();
		while ( !delayObjectQueue_.isEmpty() ) {
        	DelayObject delayObj = delayObjectQueue_.peek();
    		if( (curTime - delayObj.enqueTime_) >= DELAY_INTERVAL ) {
    			delegate_.onKaraokeFrameParsed(delayObj.flvFrameObj_.frame, delayObj.flvFrameObj_.length, true); //send it to the delayed queue
    			delayObjectQueue_.remove();
    		} else {
    			break;
    		}
    	}
	}
	//try to empty delayObjectQueue, needs at least 100ms
	private long emptyDelayObjectQueue() {
        long startTime = System.currentTimeMillis();
		try{
    		while(!delayObjectQueue_.isEmpty()) {
    			checkForDelaySend();
    			Thread.sleep( 10 );
    		}
		} catch (Exception ex) {
      		log.info("General exception:  {}", ex);
	    } 
		return (System.currentTimeMillis()-startTime);
	}
	
	private void readSongMappingTable() {
		Properties prop = new Properties();
        InputStream in;
		try {
			in = new FileInputStream(karaokeFilePath_ + "/karaoke.properties");
	        prop.load(in);
	        Enumeration<?> e = prop.propertyNames();
			while (e.hasMoreElements()) {
				String fileName = (String) e.nextElement();
				String songName = prop.getProperty(fileName);
				songMappingTable_.put(songName, fileName);
				curSong_ = karaokeFilePath_+"/"+fileName+".flv";
				log.info("-------Reading song property file: Key : {}, Value : {}", fileName, songName);
			}
	        in.close();
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	public void selectSong(String songName) {
		String fileName = songMappingTable_.get(songName);
		if(fileName != null ) {
			curSong_ = karaokeFilePath_+"/"+fileName+".flv";
			bCancelCurrentSong.set(true);
			log.info("-------A song selected: Key : {}, Value : {}", fileName, songName);
		}
	}
}
