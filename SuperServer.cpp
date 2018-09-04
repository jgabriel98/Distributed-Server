/** @author: João Gabriel Silva Fernandes
 * @email: jgabsfernandes@gmail.com
 * 
*/

#include <stdio.h>

#include "SuperServer.h"



using namespace std;


SuperServer::SuperServer(int porta, vector<int> portaSubServers) : Server(porta) {

    subSockfd.resize(portaSubServers.size());
    addr.resize(portaSubServers.size());

    //conecta-se como cliente, em cada um dos subServidores
    for(int i=0; i<portaSubServers.size(); i++){
        //socket ipv6, do tipo stream (TCP)
        subSockfd[i] = socket(AF_INET6, SOCK_STREAM, 0);
        if (subSockfd[i] < 0){
            perror(RED("Erro ao criar socket\n"));
            exit(2);
        }
        printf(GRN("Socket para SubServer criado\n"));

        //inicializando valores de addr.sin6
        memset(&addr[i], 0, sizeof(addr));
        addr[i].sin6_family = AF_INET6;
        /*if (strcmp(server_addr, "localhost") == 0)
            addr[i].sin6_addr = in6addr_any;
        else if (inet_pton(AF_INET6, "2804:7f3:386:6841::2", &addr[i].sin6_addr) <= 0){
            printf(RED("erro ao converter endereço do servidor"));
            exit(2);
        }*/
        addr[i].sin6_addr = in6addr_any;

        char str[256] = {0};
        cout << "server Addr: " << inet_ntop(AF_INET6, &(addr[i].sin6_addr), str, 256) << endl;

        addr[i].sin6_port = htons(portaSubServers[i]);
    }
    
}

void SuperServer::atenderCliente(int connectionfd, sockaddr_in6 clientAddr, socklen_t addrLen){
    char addr_str[INET6_ADDRSTRLEN];
    getpeername(connectionfd, (struct sockaddr *)&clientAddr, &addrLen); //obtem o endereço do cliente conectado

    //converte o endereçoe porta de rede em string
    if (inet_ntop(AF_INET6, &clientAddr.sin6_addr, addr_str, INET6_ADDRSTRLEN) != NULL)
    {
        cout << "\tEndereço do cliente: " << addr_str << endl;
        cout << "\tPorta do cliente: " << ntohs(clientAddr.sin6_port) << endl;
    }

    bool continuar = true;
    while (continuar){

        char buffer[2056] = {0};
        int totalBytes = recv(connectionfd, buffer, sizeof(buffer), 0);

        switch (totalBytes){
        case -1:
            perror(RED("erro ao receber mensagem do cliente"));
            continue;
        case 0:
            perror(YEL("o cliente fechou a conexão antes que todos os dados fossem enviados"));
            continuar = false;
            continue;
        default:
            cout << totalBytes << " Bytes recebidos --> ";
            printf("\"%s\"\n", buffer);
        }

        if (!strcmp(buffer, "bye") || !strcmp(buffer, "exit")){ //cliente quer parar a conexão
            continuar = false;
            strcpy(buffer, "Bye!");
            send(connectionfd, buffer, strlen(buffer) + 1, MSG_NOSIGNAL);
            shutdown(connectionfd, SHUT_RD); //fecha socket para recebimento de dados
        }
        else{
            //totalBytes = send(connectionfd, buffer, strlen(buffer) + 1, MSG_NOSIGNAL);
            totalBytes = PassMsg(connectionfd, buffer, strlen(buffer) + 1 ,MSG_NOSIGNAL);   //passa a mensagem para algum subservidor
            if (totalBytes == -1){
                perror(YEL("Erro ao reeencaminhar mensagem"));
                break;
            }
            else
                cout << CYN("mensagem reeencaminhada") << endl;
        }
        
    }

    shutdown(connectionfd, SHUT_RDWR); //fecha socket para receber e enviar dados
    printf(BOLD(YEL("Fim da conexão\n")));
}

void SuperServer::Start(){
    //"amarra"/seta o socket
    int bind_result = bind(Server::sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (bind_result < 0){
        perror(RED("bind() falhou"));
        return;
    }

    //inicia uma thread para cada subservidor
    Sthreads.resize(subSockfd.size());
    for (int i = 0; i < Sthreads.size(); i++){
        Sthreads[i] = thread(&SuperServer::HandleAnswers, this, i);
    }
}


void SuperServer::Accept(){
    //accept an incoming connection request, bloqueia o processo esperando pela conexão
    if ((connectionfd = accept(Server::sockfd, NULL, NULL)) < 0){
        perror(RED("accept() falhou"));
        return;
    }
    cout << BOLD(GRN("Cliente conectado")) << endl;

    this->atenderCliente(connectionfd, clientAddr, addrLen);
}


void SuperServer::Connect(){
    for (int i = 0; i < subSockfd.size(); i++){
        if (connect(subSockfd[i], (struct sockaddr *)&addr[i], sizeof(struct sockaddr_in6))){
            perror(RED("Nao foi possivel se conectar ao servidor"));
            close(subSockfd[i]);
            exit(2);
        }
        printf(GRN("Conectado ao servidor, porta %i\n"), addr[i].sin6_port);
    }
}

size_t SuperServer::PassMsg(int connectionFileDescriptor, char msg[], size_t buffer_len, int FLAG){
    int SubServerIdx =0;

    //enviar o descritor da conexão + mensagem original
    char _msg[buffer_len + sizeof(int)];
    memcpy(_msg, &connectionFileDescriptor, sizeof(int));
    memcpy(_msg + sizeof(int), msg, buffer_len);

    size_t len = send(subSockfd[SubServerIdx], _msg, sizeof(_msg), FLAG);

    return len;
}

//espera por uma resposta do determinado subServidor
void SuperServer::HandleAnswers(size_t subServerIdx){
    constexpr size_t buff_size = 1024;
    char bufferIn[buff_size];
    char bufferOut[buff_size];

    size_t total_bytes;
    int clientSockFd;

    while(true){
        total_bytes = recv(subSockfd[subServerIdx], bufferIn, buff_size, 0);
        
        memcpy(&clientSockFd, bufferIn, sizeof(int));
        switch (total_bytes){
        case -1:
            printf(YEL("erro ao receber mensagem do SubServidor %u"), subServerIdx);perror("");
            continue;
        case 0:
            printf(RED("o SubServidor %u fechou a conexão antes que todos os dados fossem enviados"), subServerIdx);perror((""));
            return;
        default:
            cout << total_bytes << " Bytes recebidos de SubServ. "<<subServerIdx<<" --> ";
            printf("\"%s\"\n", bufferIn+sizeof(int));
        }

        //encaminhar msg para cliente original;
        //sprintf(bufferOut, "Resposta da msg \"%s\", realizada pelo subServer %u", bufferIn+sizeof(int), subServerIdx);
        send(clientSockFd, bufferIn+sizeof(int), strlen(bufferIn+sizeof(int))+1, 0);
    }
}


void SuperServer::Close(){
    for(size_t i=0; i<subSockfd.size(); i++){
        close(subSockfd[i]);
    }
    close(Server::sockfd);
}




int main(int argc, char *argv[]){

    if (argc < 2){
        cout << YEL("informe a porta do socket ( ex: ./prog_name <porta>)\n");
        exit(1);
    }

    printf("Conectando-se a %d SubServidores\n", argc-2);

    vector<int> subServersPorts(argc-2);
    for(int i=2; i<argc; i++){
        subServersPorts[i-2] = atoi(argv[i]);
    }

    try{
        //criando objeto servidor
        class SuperServer servidor(atoi(argv[1]), subServersPorts);

    /*********conectando aos subServidores******************/

    servidor.Connect();

    /***************Iniciando servidor*********************/

        servidor.Start();
        servidor.Listen();

        while (true)
            servidor.Accept();

    }
    catch (exception ex){
        cout << "Erro ao construir objeto" << endl;
    }


    return 0;
}