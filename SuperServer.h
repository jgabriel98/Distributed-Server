#include "Server.h"


#define NUM_LISTENER_THREADS 3

class SuperServer;

class SuperServer: public Server/*, public Client*/{

    vector<int> subSockfd;
    vector<sockaddr_in6> addr;
    int connectionsPoll;
    //vector<queue<Client_connection>> filas;     //uma fila para cada sub servidor


    vector<thread> sendThreads;

    void atenderClientes();
    size_t PassMsg(int connectionFileDescriptor,char msg[], size_t buffer_len, int FLAG);
    void HandleAnswers(size_t subServerIdx);
    void ConnectToSubServers();

    static void my_signal_handler(int signal);
    static SuperServer *instance;

  public:
    SuperServer(int porta, vector<int> portaSubServers);
    ~SuperServer();
    virtual void Start();
    virtual void Listen(size_t buffer_connection_size, size_t numThreads);
    virtual void Accept();
    virtual void Close();
};
