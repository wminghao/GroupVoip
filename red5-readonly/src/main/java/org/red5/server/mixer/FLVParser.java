package org.red5.server.mixer;

import java.nio.ByteBuffer;
import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.slf4j.Logger;

public class FLVParser {
	private static final int SCAN_HEADER_TAG = 0;
	private static final int SCAN_REMAINING_TAG = 1;
	private int scanState_ = SCAN_HEADER_TAG;
	ByteBuffer curBuf_ = ByteBuffer.allocate(1<<20); //1 Meg of memory
	byte[] dataSizeStr_ = new byte[4];
    private int curFlvTagSize_;
    private int curFlvTagTimestamp_;
    private int curLen_ = 0;
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
	
	Delegate delegate_;
    public interface Delegate {
        public void onKaraokeFrameParsed(ByteBuffer frame, int len, int timestamp);
    }
    public FLVParser(FLVParser.Delegate delegate) {
    	this.delegate_ = delegate;
    }
	public void readData(byte[] src, int srcLen) {
    	int srcIndex = 0; //index into data byte array
	    while( srcLen > 0 ) {
	        switch( scanState_ ) {
	        case SCAN_HEADER_TAG:
	            {
	            	if ( curLen_ < 11 ) {
                        int cpLen = Math.min(srcLen, 11-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string                                                                                                                 
                        srcLen -= cpLen;
                        srcIndex += cpLen; //advance
                        curLen_ += cpLen;
                    }
	                if ( curLen_ >= 11 ) {
	                    curFlvTagSize_ = (curBuf_.array()[1] & 0xFF) << 16 |
	                            		 (curBuf_.array()[2] & 0xFF) << 8  |
	                            		 (curBuf_.array()[3] & 0xFF) ;
	                    curFlvTagSize_ += 4; //add previousTagLen
	                    
	                    curFlvTagTimestamp_ = (curBuf_.array()[4] & 0xFF) << 16 |
                       		 				  (curBuf_.array()[5] & 0xFF) << 8  |
                       		 				  (curBuf_.array()[6] & 0xFF);
	                    
	                    scanState_ = SCAN_REMAINING_TAG;
                    	//log.info("---got the frame info: first byte={} curFlvTagTimestamp_={} curFlvTagSize_={}", curBuf_.array()[0], curFlvTagTimestamp_, curFlvTagSize_);
	                }
	                break;
	            }
	        case SCAN_REMAINING_TAG:
	            {
                    if ( curLen_ < (curFlvTagSize_+11) ) {
                        int cpLen = Math.min(srcLen, curFlvTagSize_+11-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string     
                                      
                        srcLen -= cpLen;
                        srcIndex += cpLen;
                        curLen_+=cpLen;
                    }
                    if ( curLen_ >= (curFlvTagSize_+11) ) {
                    	//log.info("---read the frame, curFlvTagTimestamp_={} curFlvTagSize_={}", curFlvTagTimestamp_, curFlvTagSize_);
                    	//read the actual buffer
                        if( curFlvTagSize_ > 0 ) {
                        	curBuf_.flip();                      	
                            delegate_.onKaraokeFrameParsed( curBuf_, curFlvTagSize_-4+11, curFlvTagTimestamp_ );
                        }
	                    curBuf_.clear();
	                    curLen_ = 0;
	                    curFlvTagSize_  = 0; //reset and go to the first state
	                    scanState_ = SCAN_HEADER_TAG;
	                }
	                break;
	            }
	        }
	    }
	}
}
