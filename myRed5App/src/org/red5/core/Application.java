package org.red5.core;

import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.red5.server.adapter.ApplicationAdapter;
import org.red5.server.api.IClient;
import org.red5.server.api.IConnection;
import org.red5.server.api.scope.IScope;
import org.red5.server.api.Red5;
import org.red5.server.api.service.IPendingServiceCall;
import org.red5.server.api.service.IPendingServiceCallback;
import org.red5.server.api.service.IServiceCapableConnection;
import org.red5.server.api.stream.IBroadcastStream;
import org.red5.server.api.stream.IPlayItem;
import org.red5.server.api.stream.IPlaylistSubscriberStream;
import org.red5.server.api.stream.IStreamAwareScopeHandler;
import org.red5.server.api.stream.ISubscriberStream;

public class Application extends ApplicationAdapter implements
		IPendingServiceCallback, IStreamAwareScopeHandler {

    protected static Logger log = LoggerFactory.getLogger(Application.class);
    
    private Set<String> publisherList = new HashSet<String>();

	/** {@inheritDoc} */
    @Override
	public boolean appStart(IScope scope) {
		// init your handler here
		return true;
	}

	/** {@inheritDoc} */
    @Override
	public boolean appConnect(IConnection conn, Object[] params) {
		IServiceCapableConnection service = (IServiceCapableConnection) conn;
		log.info("Client connected {} conn {}", new Object[]{conn.getClient().getId(), conn});
		service.invoke("setId", new Object[] { conn.getClient().getId() },
						this);
		//notify clients of all stream published, in comma deliminated form
		String publisherListNames = "";
		int totalPublishers = publisherList.size();
		int i = 0;
		for (String publisherName : publisherList) {
			publisherListNames += publisherName;
			if( (++i) < totalPublishers) {
				publisherListNames += ",";
			}
		}
        sendToClient(conn, "initStreams", publisherListNames);
		return true;
	}

	/** {@inheritDoc} */
    @Override
	public boolean appJoin(IClient client, IScope scope) {
		log.info("Client joined app {}", client.getId());
		// If you need the connecion object you can access it via.
		IConnection conn = Red5.getConnectionLocal();
		return true;
	}

	/** {@inheritDoc} */
    public void streamPublishStart(IBroadcastStream stream) {
    	// Notify all the clients that the stream had been started
    	if (log.isDebugEnabled()) {
    		log.debug("stream broadcast start: {}", stream.getPublishedName());
    	}
    	IConnection current = Red5.getConnectionLocal();
        for(Set<IConnection> connections : scope.getConnections()) {
            for (IConnection conn: connections) {
                if (conn.equals(current)) {
                    // Don't notify current client
                    continue;
                }

                sendToClient(conn, "addStream", stream.getPublishedName());
            }
        }
        publisherList.add(stream.getPublishedName());
    }

	/** {@inheritDoc} */
    public void streamRecordStart(IBroadcastStream stream) {
	}

	/** {@inheritDoc} */
    public void streamBroadcastClose(IBroadcastStream stream) {
    	//notify the connections that a stream is unpublished
    	IConnection current = Red5.getConnectionLocal();
        for(Set<IConnection> connections : scope.getConnections()) {
            for (IConnection conn: connections) {
                if (conn.equals(current)) {
                    // Don't notify current client
                    continue;
                }
                sendToClient(conn, "removeStream", stream.getPublishedName());
            }
		}

        publisherList.remove(stream.getPublishedName());
    	super.streamBroadcastClose(stream);
	}

	/** {@inheritDoc} */
    public void streamBroadcastStart(IBroadcastStream stream) {
	}

	/** {@inheritDoc} */
    public void streamPlaylistItemPlay(IPlaylistSubscriberStream stream,
			IPlayItem item, boolean isLive) {
	}

	/** {@inheritDoc} */
    public void streamPlaylistItemStop(IPlaylistSubscriberStream stream,
			IPlayItem item) {

	}

	/** {@inheritDoc} */
    public void streamPlaylistVODItemPause(IPlaylistSubscriberStream stream,
			IPlayItem item, int position) {

	}

	/** {@inheritDoc} */
    public void streamPlaylistVODItemResume(IPlaylistSubscriberStream stream,
			IPlayItem item, int position) {

	}

	/** {@inheritDoc} */
    public void streamPlaylistVODItemSeek(IPlaylistSubscriberStream stream,
			IPlayItem item, int position) {

	}

	/** {@inheritDoc} */
    public void streamSubscriberClose(ISubscriberStream stream) {

	}

	/** {@inheritDoc} */
    public void streamSubscriberStart(ISubscriberStream stream) {
	}

	/**
	 * Get streams. called from client
	 * @return iterator of broadcast stream names
	 */
	public Set<String> getStreams() {
		IConnection conn = Red5.getConnectionLocal();
		return (Set<String>) getBroadcastStreamNames(conn.getScope());
	}

	/**
	 * Handle callback from service call. 
	 */
	public void resultReceived(IPendingServiceCall call) {
		log.info("Received result {} for {}", new Object[]{call.getResult(), call.getServiceMethodName()});
	}
	
	private void sendToClient(IConnection conn, String methodName, String param) {
		if (conn instanceof IServiceCapableConnection) {
            ((IServiceCapableConnection) conn).invoke(methodName,
                    new Object[] { param }, this);
            if (log.isDebugEnabled()) {
                log.debug("sending {} notification to {}", methodName, conn);
            }
        }
	}

	/*
	 * Notification when a song is playing
	 */
    public void onSongPlaying(String songName) {
    	super.onSongPlaying(songName);
        for(Set<IConnection> connections : scope.getConnections()) {
            for (IConnection conn: connections) {
                sendToClient(conn, "songSelected", songName);
            }
        }
    }

}