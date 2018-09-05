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


    vector<thread> Sthreads;


  public:
    //configura e inicializa o socket
    virtual void Start() = 0;

    //passa a ouvir o socket
    virtual void Listen(size_t listen_buffer_size){
      //listen for a connection request
      if (listen(sockfd, listen_buffer_size) < 0){
        perror(RED("listen() falhou"));
        return;
      }
      cout << GRN("Pronto para conexão") << endl;
    };
    //começa a operar realmente
    virtual void Accept() = 0;

    virtual void Close() = 0;

};
