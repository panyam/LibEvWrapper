
#ifndef __SERVER_H__
#define __SERVER_H__

/**
 * A socket server that can be running on the event loop.
 */
typedef struct SSSocketServer SSSocketServer;
typedef struct SSRunLoop SSRunLoop;
typedef struct SSConnection SSConnection;

/**
 * Gets the connection context corresponding to a particular connection object.
 */
extern void *ss_connection_get_context(SSConnection *connection);

/**
 * Indicates that connection has data to be written so that it will be queried for the data 
 * to send to the client.
 */
extern void ss_connection_set_writeable(SSConnection *connection);

/**
 * Closes the connection.
 */
extern void ss_connection_close(SSConnection *connection);

typedef struct SSSocketServerListener {
    /**
     * Called when a connection was accepted.  This is an opportunity
     * for the handler of this method to create any connection specific data
     * to be created and returned so that any further activities on the connection
     * will be invoked on this object.
     *
     * This method must NOT return NULL.  If it returns NULL, then the connection is refused.
     */
    void *(*createConnectionContext)();

    /**
     * Called when data has been received for this connection from a client.
     */
    void (*processData)(SSConnection *connection, const char *data, size_t length);

    /**
     * Called to indicate connection was closed.
     */
    void (*connectionClosed)(SSConnection *connection);

    /**
     * Called to ask the connection handler for data that can be written.
     * The buffer is an output parameters to be updated by the listener.
     * Return the number of bytes available in the buffer.
     */
    size_t (*writeDataRequested)(SSConnection *connection, const char **buffer);

    /**
     * Called to indicate that nWritten bytes of data has been written and that the connection
     * object should update its write buffers to discard this data.
     */
    void (*dataWritten)(SSConnection *connection, size_t nWritten);
} SSSocketServerListener;

/**
 * Create a new runloop.
 */
extern SSRunLoop *ss_runloop_new();
extern void ss_runloop_start(SSRunLoop *loop);

/**
 * Start server on a particular host/port.
 */
extern SSSocketServer *ss_start_server(const char *host, int port, SSSocketServerListener *listener);

/**
 * Stop the server running on a particular host/port.
 */
extern void ss_stop_server(SSRunLoop *loop, SSSocketServer *server);

#endif
