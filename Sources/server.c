
#include <ev.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server.h"

#define DEFAULT_READ_BUFFER_SIZE 8192

struct LEWSocketServer {
    // A pointer to the event loop that is handling event on 
    // this server socket.
    struct ev_loop *eventLoop;

    // Host the server socket is bound to (not used for now)
    char *host;

    // Port the server is running on
    int port;

    // The socket associated with the server.
    int serverSocket;
    struct ev_io acceptWatcher;

    // A listener structure for events on this socket.
    LEWSocketServerListener *listener;
};

typedef struct LEWConnection {
    /**
     * User specific connection data used to identify a connection.
     */
    void *connectionContext;

    /**
     * Client socket.
     */
    int clientSocket;    

    /**
     * Size of the read buffer.
     */
    char readBuffer[DEFAULT_READ_BUFFER_SIZE];
    size_t readBufferSize;

    /**
     * Watcher for read events on this connection.
     */
    ev_io readWatcher;

    /**
     * Watcher for write events on this connection.
     */
    ev_io writeWatcher;

    /**
     * Watcher for close events on this connection.
     */
    ev_io closeWatcher;

    /**
     * The server this connection belongs to.
     */
    LEWSocketServer *server;
} LEWConnection;

/**
 * Gets the connection context corresponding to a particular connection object.
 */
void *lew_connection_get_context(LEWConnection *connection)
{
    return connection->connectionContext;
}

/**
 * Indicates that connection has data to be written so that it will be queried for the data 
 * to send to the client.
 */
void lew_connection_set_writeable(LEWConnection *connection)
{
    ev_io_start(connection->server->eventLoop, &connection->writeWatcher);
}

/**
 * Clears the writeability of a connection.
 */
void lew_connection_clear_writeable(LEWConnection *connection)
{
    ev_io_stop(connection->server->eventLoop, &connection->writeWatcher);
}

/**
 * Closes the connection.
 */
void lew_connection_close(LEWConnection *connection)
{
    close(connection->clientSocket);
}

static void connection_read_callback(struct ev_loop *loop, struct ev_io *watcher, int revents);
static void connection_write_callback(struct ev_loop *loop, struct ev_io *watcher, int revents);
static void server_accept_callback(struct ev_loop *loop, struct ev_io *watcher, int revents);

LEWSocketServer *lew_start_server(const char *host, int port, LEWSocketServerListener *listener)
{
    int serverSocket;
    if ((serverSocket = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("Socket Error");
        return NULL;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*) &addr, sizeof(addr)) != 0)
    {
        close(serverSocket);
        perror("Bind error");
        return NULL;
    }

    if (listen(serverSocket, 32) < 0)
    {
        close(serverSocket);
        perror("Listen error");
        return NULL;
    }

    struct ev_loop *loop = ev_default_loop(0);
    LEWSocketServer *out = calloc(1, sizeof(LEWSocketServer));
    out->host = host ? strdup(host) : NULL;
    out->port = port;
    out->eventLoop = loop;
    out->listener = listener;
    out->serverSocket = serverSocket;
    out->acceptWatcher.data = out;

    ev_io_init(&out->acceptWatcher, server_accept_callback, out->serverSocket, EV_READ);
    ev_io_start(loop, &out->acceptWatcher);

    while (1)
    {
        ev_loop(loop, 0);
    }

    return out;
}

static void server_accept_callback(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int clientSocket;

    if (EV_ERROR & revents)
    {
        perror("got invalid event");
        return;
    }

    // Accept client request
    clientSocket = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
    if (clientSocket < 0)
    {
        perror("accept error");
        return;
    }

    LEWSocketServer *server = (LEWSocketServer *)watcher->data;
    void *connectionData = server->listener->connectionAccepted();
    if (connectionData)
    {
        LEWConnection *connection = (LEWConnection *)calloc(1, sizeof(LEWConnection));
        connection->readBufferSize = DEFAULT_READ_BUFFER_SIZE;
        connection->clientSocket = clientSocket;
        connection->readWatcher.data = connection;
        connection->writeWatcher.data = connection;
        connection->connectionContext = connectionData;
        connection->server = server;

        // Initialize and start watcher to read client requests
        ev_io_init(&connection->readWatcher, connection_read_callback, connection->clientSocket, EV_READ);
        ev_io_start(loop, &connection->readWatcher);
        ev_io_init(&connection->writeWatcher, connection_write_callback, connection->clientSocket, EV_WRITE);
        printf("Connection accepted on port: %d\n", clientSocket);
    } else {
        printf("Listener returned no connection.  Closing connection\n");
        close(clientSocket);
    }
}

static void connection_read_callback(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    LEWConnection *connection = (LEWConnection *)watcher->data;
    ssize_t length = read(connection->clientSocket, connection->readBuffer, connection->readBufferSize);
    if (length == 0)
    {
        // closed
        ev_io_stop(loop, &connection->readWatcher);
        // Notify that connection is closed
        connection->server->listener->connectionClosed(connection);

        // TODO: Cleanup connection
    } else if (length > 0)
    {
        connection->server->listener->dataRead(connection, connection->readBuffer, length);
    } else if (errno != EAGAIN)
    {
        perror("Read error");
    }
}

static void connection_write_callback(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    LEWConnection *connection = (LEWConnection *)watcher->data;
    const char *writeData = NULL;

    size_t writeLength = connection->server->listener->writeDataRequested(connection, &writeData);
    if (writeLength > 0 && writeData != NULL)
    {
        ssize_t nWritten = write(connection->clientSocket, writeData, writeLength);
        if (nWritten > 0)
        {
            connection->server->listener->dataWritten(connection, nWritten);
        }
    } else {
        // no data available so clear connection writeable
        lew_connection_clear_writeable(connection);
    }
}
