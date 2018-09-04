/** @author: João Gabriel Silva Fernandes
 * @email: jgabsfernandes@gmail.com
 * 
*/

#include <stdio.h>
#include <vector>
#include <queue>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

#include "color.h"

//tamanho da fila produtor consumidor - threads
#define TAM_FILA 10
#define NUM_THREADS 5

using namespace std;

class Server;

typedef struct
{
    int connectionfd;
    sockaddr_in6 clientAddr;
    socklen_t addrLen;
} Client_connection;

//baseado em : https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_72/rzab6/xacceptboth.htm



class Server
{

  protected:
    int instanceIdx;    //indice da instancia do servidor, posso remover depois

    int Domain = AF_INET6;  //usando ipv6
    int type = SOCK_STREAM; //usando TCP
    int ipProtocol = 0;     //usando protocolo IP (nao mudar)

    int sockfd;         //descritor de arquivo do socket
    int connectionfd;   //descritor de arquivo da conexão
    sockaddr_in6 serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    static const size_t filaEsperaMaxSize = 10; //tamanho da fila do dispatcher

    vector<thread> Sthreads;
    queue<Client_connection> fila;

    pthread_cond_t consCond = PTHREAD_COND_INITIALIZER;
    pthread_cond_t prodCond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;

    static void atenderCliente(int connectionfd, sockaddr_in6 clientAddr, socklen_t addrLen);
    void consumir(int num_thrd);

  public:
    Server(){}
    Server(int porta);

    //configura e inicializa o socket
    void Start();

    //passa a ouvir o socket
    void Listen();
    //começa a operar realmente
    void Accept();

    void Close();
    ~Server();


    //handler para fechar sockets de todos os servers quando receber sinal ctrl+c
    static void my_handler(int s);

};
