#include "SubServer.h"
using namespace std;


static SubServer *instance;

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
    serverAddr.sin6_addr = in6addr_any; //ou gethostbyname("localhost");

    cout << GRN("Socket criado") << endl;

    /*********configura o signal***********/
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = &SubServer::my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    /**************************************/

    instance = this;
}

void SubServer::atenderServer(int connectionfd){

    char bufferIn[2056] = {0};
    char bufferOut[2056] = {0};
    char *msg;

    int totalBytes;
    bool continuar = true;
    
    while(continuar){
        //recebe mesagem e já "desencapsula" ela (4 primeiros bytes são um inteiro descritor da conexão)
        totalBytes = recv(connectionfd, bufferIn, sizeof(bufferIn), 0);
        msg = bufferIn+sizeof(int);

        switch (totalBytes){
            case -1:
                perror(RED("erro ao receber mensagem do SuperServidor"));
                send(connectionfd, bufferIn, 0, MSG_NOSIGNAL);
                break;
            case 0:
                perror(YEL("o SuperServidor fechou a conexão antes que todos os dados fossem encaminhados"));
                continuar = false;
                break;
            default:
                cout << totalBytes << " Bytes recebidos --> ";
                printf("\"%s\"\n", msg);

                /********trata do processamento da requisição********/


                /*******terminar de tratar requisição***************/

                //envia a mensagem processada de volta ao servidor
                memcpy(bufferOut, bufferIn, sizeof(int));
                sprintf(bufferOut+sizeof(int), "Resposta da msg \"%s\", realizada pelo subServer da porta %hu", msg, ntohs((ushort)serverAddr.sin6_port));
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
    printf(BOLD(YEL("Fim da conexão\n")));
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

    //Sthreads.push_back( thread(atenderCliente, connectionfd, clientAddr, addrLen) );

    //inicia as threads
    Sthreads.resize(NUM_THREADS);
    for (int i = 0; i < Sthreads.size(); i++){
        Sthreads[i] = thread(&SubServer::atenderServer, this, connectionfd);
    }
    atenderServer(connectionfd);

}

SubServer::~SubServer(){
    printf("Fechando SubServidor\n");
    Close();
}

void SubServer::Close(){
    printf("\t" BOLD("`") "->Finalizando threads 0 a %u", Sthreads.size()-1);
    //pthread_cond_destroy(&consCond);
    //pthread_cond_destroy(&prodCond);
    //pthread_mutex_destroy(&Mutex);
    close(sockfd);
    close(connectionfd);
    for (int i = 0; i < Sthreads.size(); i++){
        pthread_cancel(Sthreads[i].native_handle());
    }
}

//handler para fechar sockets de todos os servers quando receber sinal ctrl+c
void SubServer::my_handler(int s){
    printf("\nCaught signal %d\n", s);
    printf(MAG("Fechando SubServidor\n"));
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
        subServidor.Listen();

        subServidor.Accept();
    }
    catch (exception ex) {
        cout << "Erro ao construir objeto" << endl;
    }
}