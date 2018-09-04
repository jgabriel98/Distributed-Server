#include "Server.h"


class SuperServer;

class SuperServer: public Server/*, public Client*/{

    vector<int> subSockfd;
    vector<sockaddr_in6> addr;
    //vector<queue<Client_connection>> filas;     //uma fila para cada sub servidor

  public:
    SuperServer(int porta, vector<int> portaSubServers);
    void Start();
    void Connect();
    void Accept();
    void atenderCliente(int connectionfd, sockaddr_in6 clientAddr, socklen_t addrLen);
    size_t PassMsg(int connectionFileDescriptor,char msg[], size_t buffer_len, int FLAG);
    void Produzir();
    void HandleAnswers(size_t subServerIdx);
    void Close();
};
