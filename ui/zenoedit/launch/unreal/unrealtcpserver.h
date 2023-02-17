#ifndef ZENO_UNREALTCPSERVER_H
#define ZENO_UNREALTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class UnrealTcpServer final : public QObject {
    Q_OBJECT

public:
    /**
     * get singleton lazily (thread safe above c++11)
     * @return Unique instance of ['UnrealTcpServer']
     */
    static UnrealTcpServer& getStaticClass();

public:
    explicit UnrealTcpServer(QObject* parent = nullptr);
    ~UnrealTcpServer() final;

    bool isRunning();

public slots:
    /**
     * Start the server
     * @param inAddress clients from which internet interface are allowed
     * @param inPort binding port, default 23343
     */
    void start(const QHostAddress& inAddress = QHostAddress::LocalHost, int32_t inPort = 23343);
    /**
     * shutdown and release server instance
     */
    void shutdown();

private slots:
    /**
     * when new connection established
     */
    void onNewConnection();

    /**
     * when socket disconnect
     * we set it back to nullptr
     */
     void onCurrentConnectionClosed();

     /**
      * when socket has new data
      */
      void onCurrentConnectionReceiveData();

private:
    // just running in event loop, it doesn't need to ensure thread safe
    // mutable std::shared_mutex _guard;

    // current tcp server instance
    QTcpServer* m_server = nullptr;
    // current handling socket, will be closed if there is new incoming connection.
    QTcpSocket* m_currentSocket = nullptr;

private:
    void cleanUpSocket();
};

#endif // ZENO_UNREALTCPSERVER_H
