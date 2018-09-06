/** @author: João Gabriel Silva Fernandes
 * @email: jgabsfernandes@gmail.com
 * 
*/
#include "SubServer.h"
#include <stdlib.h>
using namespace std;


SubServer* SubServer::instance = NULL;

SubServer::SubServer(int porta){

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
    
    //inet_pton(AF_INET6, "ipv6 que quer usar", &serverAddr.sin6_addr);
    serverAddr.sin6_addr = in6addr_any; //ou gethostbyname("localhost");

    cout << GRN("Socket criado") << endl;

    /*********configura o signal***********/
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = &this->my_signal_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    /**************************************/

    instance = this;
}

void SubServer::atenderServer(int connectionfd, int threadNum){

    char bufferIn[2056] = {0};
    char bufferOut[2056] = {0};
    char *msg;

    int totalBytes;
    
    while(true){
        //recebe mesagem e já "desencapsula" ela (4 primeiros bytes são um inteiro descritor da conexão)
        totalBytes = recv(connectionfd, bufferIn, sizeof(bufferIn), 0);
        msg = bufferIn+sizeof(int);

        switch (totalBytes){
            case -1:
                perror(RED("erro ao receber mensagem do SuperServidor"));
                send(connectionfd, bufferIn, 0, MSG_NOSIGNAL);
                continue;
            case 0:     //serviord Super desconectou-se de mim
                //perror(YEL("o SuperServidor fechou a conexão antes que todos os dados fossem encaminhados"));
                return;
            default:
                cout << totalBytes << " Bytes recebidos --> ";
                printf("\"%s\"\n", msg);

                /********trata do processamento da requisição********/
                //hardwork();


                /*******terminar de tratar requisição***************/

                //envia a mensagem processada de volta ao servidor
                memcpy(bufferOut, bufferIn, sizeof(int));
                sprintf(bufferOut + sizeof(int), "Resposta da msg \"%s\", realizada pelo subServer da porta %hu, thread %d", msg, ntohs((ushort)serverAddr.sin6_port), threadNum);
                msg = bufferOut+sizeof(int);
                
                totalBytes = send(connectionfd, bufferOut, strlen(msg) + 1 + sizeof(int), MSG_NOSIGNAL);
                if (totalBytes == -1)
                    cout << YEL("Falhou ao responder, SuperServidor fechou a conexão") << endl;
                else
                    cout << CYN("resposta enviada") << endl;
                //break;
        }

    }

    //shutdown(connectionfd, SHUT_RDWR); //fecha socket para receber e enviar dados
    //printf(BOLD(YEL("Fim da conexão\n")));
    printf(BOLD(YEL("Fim da trhead %d\n")), threadNum);
}

//configura e inicializa o socket
void SubServer::Start(){
    //"amarra"/seta o socket
    int bind_result = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (bind_result < 0){
        perror(RED("bind() falhou"));
        return;
    }
}

void SubServer::Listen(size_t listen_buffer_size = 20){
    Server::Listen(listen_buffer_size);
}

void SubServer::Accept(){
    //accept an incoming connection request, bloqueia o processo esperando pela conexão
    if ((connectionfd = accept(sockfd, NULL, NULL)) < 0){
        perror(RED("accept() falhou"));
        return;
    }
    cout << BOLD(GRN("SuperServidor conectado")) << endl;

    char addr_str[INET6_ADDRSTRLEN];
    getpeername(connectionfd, (struct sockaddr *)&clientAddr, &addrLen); //obtem o endereço do cliente conectado

    //converte o endereçoe porta de rede em string
    if (inet_ntop(AF_INET6, &clientAddr.sin6_addr, addr_str, INET6_ADDRSTRLEN) != NULL){
        cout << "\tEndereço do SuperServidor: " << addr_str << endl;
        cout << "\tPorta da conexão: " << ntohs(clientAddr.sin6_port) << endl;
    }

    //inicia as threads
    Sthreads.resize(NUM_THREADS);
    for (int i = 0; i < Sthreads.size(); i++)
        Sthreads[i] = thread(&SubServer::atenderServer, this, connectionfd, i+1);
    
    atenderServer(connectionfd, 0); //faz a thread main tb trabalhar

}

SubServer::~SubServer(){
    printf(MAG("Fechando SubServidor\n"));
    Close();
}

void SubServer::Close(){
    //printf("\t" BOLD("`") "->Finalizando threads 0 a %lu", Sthreads.size()-1);
    for (thread &thrd: Sthreads)
        thrd.join();    // ou pthread_cancel(thrd.native_handle()); ... dependendo do caso
    close(sockfd);
    close(connectionfd);
}

//handler para fechar sockets de todos os servers quando receber sinal ctrl+c
void SubServer::my_signal_handler(int s){
    printf("\nCaught signal %d\n", s);
    printf(MAG("Fechando SubServidor\n"));
    
    printf("\t" BOLD("`") "->Finalizando threads 0 a %lu", instance->Sthreads.size()-1);
    for (thread &thrd: instance->Sthreads)
        pthread_cancel(thrd.native_handle());

    instance->Close();
    printf("\n");
    exit(1);
}


//para matar processo de uma porta: fuser -n tcp -k 9000

int main(int argc, char *argv[]){

    if (argc < 2){
        cout << YEL("informe a porta do socket ( ex: ./prog_name <porta>)\n");
        exit(1);
    }

    try {
        //criando objeto servidor
        class SubServer subServidor(atoi(argv[1]));

        /***************Iniciando servidor*********************/
        subServidor.Start();
        subServidor.Listen(20);

        subServidor.Accept();
    }
    catch (exception ex) {
        cout << "Erro ao construir objeto" << endl;
    }

}