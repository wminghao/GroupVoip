package org.red5.server.mixer;

import java.nio.ByteBuffer;

public class FLVParser {
	private static final int SCAN_HEADER_TAG = 0;
	private static final int SCAN_REMAINING_TAG = 1;
	private int scanState_ = SCAN_HEADER_TAG;
	ByteBuffer curBuf_ = ByteBuffer.allocate(1<<20); //1 Meg of memory
	byte[] dataSizeStr_ = new byte[4];
    private int curFlvTagSize_;
    private int curFlvTagTimestamp_;
    private int curLen_;
	
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
	                    curFlvTagSize_ = (0 & 0xFF) |
	                            		 (curBuf_.array()[1] & 0xFF) << 8 |
	                            		 (curBuf_.array()[2] & 0xFF) << 16 |
	                            		 (curBuf_.array()[3] & 0xFF) << 24;
	                    curFlvTagSize_ += 4; //add previousTagLen
	                    
	                    curFlvTagTimestamp_ = (0 & 0xFF) |
                       		 (curBuf_.array()[4] & 0xFF) << 8 |
                       		 (curBuf_.array()[5] & 0xFF) << 16 |
                       		 (curBuf_.array()[6] & 0xFF) << 24;
	                    
                        curBuf_.clear();
                        curLen_ = 0;
	                    scanState_ = SCAN_REMAINING_TAG;
	                }
	                break;
	            }
	        case SCAN_REMAINING_TAG:
	            {
                    if ( curLen_ < curFlvTagSize_ ) {
                        int cpLen = Math.min(srcLen, curFlvTagSize_-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string     
                                      
                        srcLen -= cpLen;
                        srcIndex += cpLen;
                        curLen_+=cpLen;
                    }
                    if ( curLen_ >= curFlvTagSize_ ) {
                    	//read the actual buffer
                        if( curFlvTagSize_ > 0 ) {
                        	curBuf_.flip();
                            delegate_.onKaraokeFrameParsed( curBuf_, curFlvTagSize_-4, curFlvTagTimestamp_ ); 
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
