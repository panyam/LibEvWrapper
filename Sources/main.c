
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"

typedef struct EchoConnection {
    char readBuffer[4096];
    int length;
} EchoConnection;

/**
 * Called when a connection was accepted.  This is an opportunity
 * for the handler of this method to create any connection specific data
 * to be created and returned so that any further activities on the connection
 * will be invoked on this object.
 *
 * This method must NOT return NULL.  If it returns NULL, then the connection is refused.
 */
void *createConnectionContextCallback()
{
    EchoConnection *out = calloc(1, sizeof(EchoConnection));
    return out;
}

/**
 * Called when data has been received for this connection from a client.
 */
void processDataCallback(LEWConnection *connection, const char *data, size_t length)
{
    EchoConnection *echoconn = (EchoConnection *)lew_connection_get_context(connection);
    memcpy(echoconn->readBuffer, data, length);
    echoconn->length = length;
    lew_connection_set_writeable(connection);
}

/**
 * Called to indicate connection was closed.
 */
void connectionClosedCallback(LEWConnection *connection)
{
    printf("Connection closed...\n");
}

/**
 * Called to ask the connection handler for data that can be written.
 * The buffer is an output parameters to be updated by the listener.
 * Return the number of bytes available in the buffer.
 */
size_t writeDataRequestedCallback(LEWConnection *connection, const char **buffer)
{
    printf("Write data requested...\n");
    EchoConnection *echoconn = (EchoConnection *)lew_connection_get_context(connection);
    *buffer = echoconn->readBuffer;
    return echoconn->length;
}

/**
 * Called to indicate that nWritten bytes of data has been written and that the connection
 * object should update its write buffers to discard this data.
 */
void dataWrittenCallback(LEWConnection *connection, size_t nWritten)
{
    EchoConnection *echoconn = (EchoConnection *)lew_connection_get_context(connection);
    echoconn->length -= nWritten;
}

int main(void)
{
    LEWSocketServerListener listener;
    listener.createConnectionContext = createConnectionContextCallback;
    listener.processData = processDataCallback;
    listener.connectionClosed = connectionClosedCallback;
    listener.writeDataRequested = writeDataRequestedCallback;
    listener.dataWritten = dataWrittenCallback;

    lew_start_server(NULL, 9999, &listener);
}
