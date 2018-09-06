/** @author: João Gabriel Silva Fernandes
 * @email: jgabsfernandes@gmail.com
 * 
*/
#include "Server.h"


#define NUM_LISTENER_THREADS 3

class SuperServer;

class SuperServer: public Server/*, public Client*/{

    vector<bool> upServers;
    vector<int> subSockfd;
    vector<sockaddr_in6> addr;
    int connectionsPoll;  //poll para ouvir mensagens de todos clientes


    vector<thread> sendThreads; //threads que escutam o poll 'connectionsPoll'

    void atenderClientes();
    size_t PassMsg(int connectionFileDescriptor,char msg[], size_t buffer_len, int FLAG);
    void HandleAnswers(size_t subServerIdx);
    bool _reconnectToSubServer(size_t subServerIdx);
    void ConnectToSubServers();
    bool isSubServerUp(size_t subServerIdx) { return upServers[subServerIdx]; }

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
