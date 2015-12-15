
#ifndef __LEW_SERVER_H__
#define __LEW_SERVER_H__

/**
 * A socket server that can be running on the event loop.
 */
typedef struct LEWSocketServer LEWSocketServer;
typedef struct LEWRunLoop LEWRunLoop;
typedef struct LEWConnection LEWConnection;

/**
 * Gets the connection context corresponding to a particular connection object.
 */
extern void *lew_connection_get_context(LEWConnection *connection);

/**
 * Indicates that connection has data to be written so that it will be queried for the data 
 * to send to the client.
 */
extern void lew_connection_set_writeable(LEWConnection *connection);

/**
 * Closes the connection.
 */
extern void lew_connection_close(LEWConnection *connection);

typedef struct LEWSocketServerListener {
    /**
     * Called when a connection was accepted.  This is an opportunity
     * for the handler of this method to create any connection specific data
     * to be created and returned so that any further activities on the connection
     * will be invoked on this object.
     *
     * This method must NOT return NULL.  If it returns NULL, then the connection is refused.
     */
    void *(*connectionAccepted)();

    /**
     * Called when data has been received for this connection from a client.
     */
    void (*dataRead)(LEWConnection *connection, const char *data, size_t length);

    /**
     * Called to indicate connection was closed.
     */
    void (*connectionClosed)(LEWConnection *connection);

    /**
     * Called to ask the connection handler for data that can be written.
     * The buffer is an output parameters to be updated by the listener.
     * Return the number of bytes available in the buffer.
     */
    size_t (*writeDataRequested)(LEWConnection *connection, const char **buffer);

    /**
     * Called to indicate that nWritten bytes of data has been written and that the connection
     * object should update its write buffers to discard this data.
     */
    void (*dataWritten)(LEWConnection *connection, size_t nWritten);
} LEWSocketServerListener;

/**
 * Create a new runloop.
 */
extern LEWRunLoop *lew_runloop_new();
extern void lew_runloop_start(LEWRunLoop *loop);

/**
 * Start server on a particular host/port.
 */
extern LEWSocketServer *lew_start_server(const char *host, int port, LEWSocketServerListener *listener);

/**
 * Stop the server running on a particular host/port.
 */
extern void lew_stop_server(LEWRunLoop *loop, LEWSocketServer *server);

#endif
