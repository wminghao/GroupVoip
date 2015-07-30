package org.red5.server.mixer;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

//import org.red5.logging.Red5LoggerFactory;
//import org.red5.server.api.Red5;
//import org.slf4j.Logger;

class SegmentParser
{
    public interface Delegate {
        public void onFrameParsed(int mixerId, ByteBuffer frame, int len);
    }
    
	public SegmentParser(Delegate delegate) {
		this.delegate = delegate;

		 // to use little endian
		curBuf_.order(ByteOrder.LITTLE_ENDIAN);
	}
	private Delegate delegate;
	//private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
	
    private int count_bits(int n) {
    	int c; // c accumulates the total bits set in v
    	for (c = 0; n!=0; c++) {
    		n &= n - 1; // clear the least significant bit set
    	}
    	return c;
    }
    private static final int MAX_XCODING_INSTANCES = 32;
    
    //segment parsing state
    private static final int SEARCHING_SEGHEADER = 0;
    private static final int SEARCHING_STREAM_MASK = 1;
    private static final int SEARCHING_STREAM_HEADER = 2;
    private static final int SEARCHING_STREAM_DATA = 3;
    
    private int parsingState_ = SEARCHING_SEGHEADER;
    private ByteBuffer curBuf_ = ByteBuffer.allocate(1<<20); //1 Meg of memory
    private int curLen_ = 0;
    private int curStreamId_ = 0;
    private int curStreamLen_ = 0;
    private int curStreamCnt_ = 0;
    private int numStreams_ = 0;
    
    boolean readData(byte[] src, int srcLen)
    {
    	int srcIndex = 0; //index into data byte array
        while( srcLen > 0 ) {
            switch( parsingState_ ) {
            case SEARCHING_SEGHEADER:
                {
                    if ( curLen_ < 3 ) {
                        int cpLen = Math.min(srcLen, 3-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string                                                                                                                 
                        srcLen -= cpLen;
                        srcIndex += cpLen; //advance
                        curLen_+=cpLen;
                    }

                    if ( curLen_ >= 3 ) {
                        assert(curBuf_.array()[0] == 'S' && curBuf_.array()[1] == 'G' && curBuf_.array()[2] == 'O');

                        //System.out.println("---Read SGO header, len="+curLen_);
                        curBuf_.clear();
                        curLen_ = 0;
                        parsingState_ = SEARCHING_STREAM_MASK;
                    }
                    break;
                }
            case SEARCHING_STREAM_MASK:
                {
                    if ( curLen_ < 4 ) {
                        int cpLen = Math.min(srcLen, 4-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string
                        srcLen -= cpLen;
                        srcIndex += cpLen; //advance
                        curLen_+=cpLen;
                    }

                    if ( curLen_ >= 4 ) {
                    	curBuf_.flip();
                        int streamMask = curBuf_.getInt();
                        
                        //handle mask here 
                        numStreams_ = count_bits(streamMask)+1;
                        assert(numStreams_ < MAX_XCODING_INSTANCES);
                        
                        //log.info("---streamMask={} numStreams_={}", streamMask, numStreams_);
                        //System.out.println("---streamMask="+streamMask+" numStreams_="+numStreams_);
                        int index = 0;
                        while( streamMask !=0 ) {
                            int value = ((streamMask<<31)>>31); //mask off all other bits
                            if( value!=0 ) {
                            	//log.info("---streamMask index={} is valid.", index);
                            	//System.out.println("---streamMask index="+index+" is valid.");
                            }
                            streamMask >>= 1; //shift 1 bit
                            index++;
                        }

                        curStreamCnt_ = numStreams_;
                        curBuf_.clear();
                        curLen_ = 0;
                        parsingState_ = SEARCHING_STREAM_HEADER;
                    }
                    break;
                }
            case SEARCHING_STREAM_HEADER:
                {
                    if ( curLen_ < 5 ) {
                        int cpLen = Math.min(srcLen, 5-curLen_);
                        curBuf_.put(src, srcIndex, cpLen);//concatenate the string
                        srcLen -= cpLen;
                        srcIndex += cpLen;
                        curLen_+=cpLen;
                    }
                    if ( curLen_ >= 5 ) {
                    	curBuf_.flip();
                        curStreamId_ = curBuf_.get();
                        assert(curStreamId_ <= MAX_XCODING_INSTANCES);                    
                        curStreamLen_ = curBuf_.getInt();
                        //log.info("---curBuf_[0]={}, curStreamId_={}, len={}\r\n", curBuf_.array()[0], curStreamId_, curStreamLen_);
                        //System.out.println("---curBuf_[0]="+curBuf_.array()[0]+", curStreamId_="+curStreamId_ + " curStreamLen_="+ curStreamLen_);
                        curBuf_.clear();
                        curLen_ = 0;
                        parsingState_ = SEARCHING_STREAM_DATA;
                    }
                    break;
                }
            case SEARCHING_STREAM_DATA:
                {
                    if ( curLen_ < curStreamLen_ ) {
                        int cpLen = Math.min(srcLen, curStreamLen_-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string     
                                      
                        srcLen -= cpLen;
                        srcIndex += cpLen;
                        curLen_+=cpLen;
                    }
                    if ( curLen_ >= curStreamLen_ ) {
                    	//log.info("---curStreamId_={} curStreamLen_={}", curStreamId_, curStreamLen_);
                    	//System.out.println("---curStreamId_="+curStreamId_+" curStreamLen_="+curStreamLen_);
                        //read the actual buffer
                        if( curStreamLen_ > 0 ) {
                        	curBuf_.flip();
                            delegate.onFrameParsed(curStreamId_, curBuf_, curStreamLen_); 
                        }
                        curBuf_.clear();
                        curLen_ = 0;
                        curStreamCnt_--;
                        if ( curStreamCnt_ > 0 ) {
                            parsingState_ = SEARCHING_STREAM_HEADER;
                        } else {
                            parsingState_ = SEARCHING_SEGHEADER;
                        }
                    }
                    break;
                }
            }
        }
        return true;
    }    
}