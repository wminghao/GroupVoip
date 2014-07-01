package org.red5.core;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.red5.server.api.IConnection;
import org.red5.server.api.Red5;
import org.red5.server.net.rtmp.RTMPConnection;

public class SongService {
    protected static Logger log = LoggerFactory.getLogger(Application.class);
	public void selectSong(String songName) {

		IConnection conn = Red5.getConnectionLocal();

        if (conn instanceof RTMPConnection) {
        	((RTMPConnection) conn).selectSong(songName);
        }
	}
}
