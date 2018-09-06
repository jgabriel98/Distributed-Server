/** @author: João Gabriel Silva Fernandes
 * @email: jgabsfernandes@gmail.com
 * 
*/

#include <stdio.h>
#include <sys/epoll.h>

#include "SuperServer.h"
#include "signal.h"



using namespace std;

SuperServer* SuperServer::instance;

SuperServer::SuperServer(int porta, vector<int> portaSubServers){

    sockfd = socket(Domain, type, ipProtocol);

    //cancela o construtor com uma exceção
    if (sockfd < 0)
    {
        perror(BOLD(RED("Erro ao criar socket\n")));
        throw("");
    }
    serverAddr.sin6_family = AF_INET6;
    // htonl() -> converte um short sem sinal de host byte order para network byte order ( o que é isso?, nao faço ideia)
    serverAddr.sin6_port = htons((ushort)porta);
    serverAddr.sin6_addr = in6addr_any; //ou gethostbyname("localhost");

    cout << GRN("Socket criado") << endl;



    /**********conexão dos subServidores********/

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
        addr[i].sin6_addr = in6addr_any; // ou entao: inet_pton(AF_INET6, "ipv6 do SuperServidor", &addr.sin6_addr);

        char str[256] = {0};
        cout << "server Addr: " << inet_ntop(AF_INET6, &(addr[i].sin6_addr), str, 256) << endl;

        addr[i].sin6_port = htons(portaSubServers[i]);
    }

    /*********configura o signal***********/
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = &this->my_signal_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    /**************************************/

    instance = this;
}

//le e processa as mensagens dos clientes. sem realação de "designar" uma thread para unico cliente ou vice-versa
void SuperServer::atenderClientes(){
    char buffer[2056] = {0};

    sigset_t signalMask;
    epoll_event event;

    while (true){
        //espera pela chegada de alguma mensagem no poll, apenas um por vez
        epoll_pwait(connectionsPoll, &event, 1, -1, &signalMask);
        int totalBytes = recv(event.data.fd, buffer, sizeof(buffer), 0);

        switch (totalBytes){
        case -1:
            perror(RED("erro ao receber mensagem do cliente"));
            continue;
        case 0:
            perror(YEL("o cliente fechou a conexão antes que todos os dados fossem enviados"));
            epoll_ctl(connectionsPoll, EPOLL_CTL_DEL, event.data.fd, &event);
            close(event.data.fd);
            continue;
        default:
            cout << totalBytes << " Bytes recebidos --> ";
            printf("\"%s\"\n", buffer);
        }

        if (!strcmp(buffer, "bye") || !strcmp(buffer, "exit")){ //cliente quer parar a conexão
            strcpy(buffer, "Bye!");
            send(event.data.fd, buffer, strlen(buffer) + 1, MSG_NOSIGNAL);
            printf(BOLD(YEL("\tFim de conexão com cliente\n")));
            epoll_ctl(connectionsPoll, EPOLL_CTL_DEL, event.data.fd, &event);
            close(event.data.fd);   //fecha o socket com cliente
        }
        else{
            
            totalBytes = PassMsg(event.data.fd, buffer, strlen(buffer) + 1 ,MSG_NOSIGNAL);   //passa a mensagem para algum subservidor
            if (totalBytes == -1)
                perror(YEL("Erro ao reencaminhar mensagem"));
            else
                cout << CYN("\tmensagem reencaminhada para SubServer\n") << endl;
        }
        
    }

    //printf(BOLD(YEL("Fim da conexão\n")));
}

void SuperServer::Start(){
    
    //conecta aos subServidores
    SuperServer::ConnectToSubServers();

    //"amarra"/seta o socket
    int bind_result = bind(Server::sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (bind_result < 0){
        perror(RED("bind() falhou"));
        return;
    }

    //inicia uma thread para cada subservidor, que vai tratar das respostas dele
    Sthreads.resize(subSockfd.size());
    for (int i=0; i<Sthreads.size(); i++){
        Sthreads[i] = thread(&SuperServer::HandleAnswers, this, i);
    }
}

void SuperServer::ConnectToSubServers(){
    for (int i = 0; i < subSockfd.size(); i++){
        if (connect(subSockfd[i], (struct sockaddr *)&addr[i], sizeof(struct sockaddr_in6))){
            perror(RED("Nao foi possivel se conectar ao servidor"));
            close(subSockfd[i]);
            exit(2);
        }
        printf(GRN("Conectado ao SubServidor %d, porta %i\n"), i,addr[i].sin6_port);
    }

}
void SuperServer::Listen(size_t buffer_connection_size = 50, size_t numThreads = NUM_LISTENER_THREADS){
    //listen for a connection request
    if (listen(sockfd, buffer_connection_size) < 0){
        perror(RED("listen() falhou"));
        return;
    }
    cout << GRN("Pronto para conexão com Clientes") << endl;

 /******adiciona socket ao poll ******/
    connectionsPoll = epoll_create1(0);

    if(connectionsPoll <0){
        perror(RED("Erro ao criar poll de sockets"));
        exit(3);
    }

    sendThreads.resize(numThreads);
    for(thread &thrd: sendThreads)
        thrd = thread(&SuperServer::atenderClientes, this);
}

void SuperServer::Accept(){
    char addr_str[INET6_ADDRSTRLEN];


    //accept an incoming connection request, bloqueia o processo esperando pela conexão
    if ((connectionfd = accept(Server::sockfd, NULL, NULL)) < 0){
        perror(RED("accept() falhou"));
        return;
    }
    cout << BOLD(GRN("Cliente conectado")) << endl;


    getpeername(connectionfd, (struct sockaddr *)&clientAddr, &addrLen); //obtem o endereço do cliente conectado
    //converte o endereçoe porta de rede em string
    if (inet_ntop(AF_INET6, &clientAddr.sin6_addr, addr_str, INET6_ADDRSTRLEN) != NULL){
        cout << "\tEndereço do cliente: " << addr_str << endl;
        cout << "\tPorta do cliente: " << ntohs(clientAddr.sin6_port) << endl;
    }



    struct epoll_event *event = new epoll_event;
    event->events = EPOLLIN | EPOLLET;
    //event->events = EPOLLIN;
    event->data.fd = connectionfd;

    if(epoll_ctl(connectionsPoll, EPOLL_CTL_ADD, event->data.fd, event)<0){
        perror(RED("Erro ao adicionar conexão ao poll"));
        return;
    }

    //this->atenderClientes(connectionfd, clientAddr, addrLen);
}


size_t SuperServer::PassMsg(int connectionFileDescriptor, char msg[], size_t buffer_len, int FLAG){
    static int SubServerIdx = 0;

    //seleciona apenas servidores em pé ( que nao cairam )
    do{
        SubServerIdx = ++SubServerIdx % subSockfd.size();
    }while(subSockfd[SubServerIdx] < 0 );

    printf("num of subServers %lu\tEnviando para %d", subSockfd.size(), SubServerIdx);

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
    //char bufferOut[buff_size];

    size_t total_bytes;
    int clientSockFd;

    while(true){
        total_bytes = recv(subSockfd[subServerIdx], bufferIn, buff_size, 0);
        
        memcpy(&clientSockFd, bufferIn, sizeof(int));
        switch (total_bytes){
        case -1:
            printf(YEL("erro ao receber mensagem do SubServidor %lu"), subServerIdx);perror("");
            continue;
        case 0:
            printf(RED("o SubServidor %lu fechou a conexão "), subServerIdx);
            fflush(stdout);

            //remove o subservidor da lista e finaliza thread ('return')
            close(subSockfd[subServerIdx]);
            subSockfd[subServerIdx] = -1; //"flag" para indicar que socket fechou
            return;
        default:
            cout << total_bytes << " Bytes recebidos de SubServ. "<<subServerIdx<<endl;
            //printf("\"%s\"\n", bufferIn+sizeof(int));
        }

        //encaminhar msg para cliente original;
        send(clientSockFd, bufferIn+sizeof(int), strlen(bufferIn+sizeof(int))+1, 0);
        printf(CYN("\tmensagem reencaminhada para Cliente\n"));
    }
}


void SuperServer::Close(){

    printf("\t" BOLD("`") "->Finalizando Sender-threads 0 a %lu\n", sendThreads.size() - 1);
    for (int i = 0; i < Sthreads.size(); i++)
        pthread_cancel(Sthreads[i].native_handle());

    printf("\t" BOLD("`") "->Finalizando recevier-threads(SubServer receiver) 0 a %lu\n", Sthreads.size() - 1);
    for (int i = 0; i < Sthreads.size(); i++)
        pthread_cancel(Sthreads[i].native_handle());
    printf("\t" BOLD("`") "->Desconectando dos subServers 0 a %lu", subSockfd.size() - 1);
    for(size_t i=0; i<subSockfd.size(); i++)
        close(subSockfd[i]);
    


    close(connectionsPoll);
    close(Server::sockfd);
    close(Server::connectionfd);
}

SuperServer::~SuperServer(){
    printf("Fechando Super Servidor\n");
    Close();
    instance = NULL;
}
//handler para fechar sockets de todos os servers quando receber sinal ctrl+c
void SuperServer::my_signal_handler(int s){
    printf("\nCaught signal %d\n", s);
    printf(MAG("Fechando Super Servidor\n"));

    (instance)->Close();
    
    printf("\n");
    exit(1);
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

    /******** Criando o Servidor main e seu socket, *****
     ******* e conectando-o aos subServidores***********/
        class SuperServer servidor(atoi(argv[1]), subServersPorts);


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