/** @author: João Gabriel Silva Fernandes
 * @email: jgabsfernandes@gmail.com
 * 
*/
#include "Server.h"

//tamanho da fila produtor consumidor - threads
#define TAM_FILA 10
#define NUM_THREADS 5

using namespace std;



//baseado em : https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_72/rzab6/xacceptboth.htm

static vector<Server *> instances;

Server::Server(int porta)
{
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

    sigIntHandler.sa_handler = &this->my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    /**************************************/

    instanceIdx = instances.size();
    instances.push_back(this); //adiciona ptr do objeto
}




//configura e inicializa o socket
void Server::Start(){
    //"amarra"/seta o socket
    int bind_result = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (bind_result < 0){
        perror(RED("bind() falhou"));
        return;
    }

    //inicia as threads
    Sthreads.resize(NUM_THREADS);
    for (int i = 0; i < Sthreads.size(); i++){
        Sthreads[i] = thread(&Server::consumir, this, i);
    }
}

//começa a operar realmente
void Server::Listen(){
    //listen for a connection request
    if (listen(sockfd, filaEsperaMaxSize) < 0){
        perror(RED("listen() falhou"));
        return;
    }
    cout << GRN("Pronto para conexão com Clientes") << endl;
}

void Server::Accept(){
    //accept an incoming connection request, bloqueia o processo esperando pela conexão
    if ((connectionfd = accept(sockfd, NULL, NULL)) < 0){
        perror(RED("accept() falhou"));
        return;
    }
    cout << BOLD(GRN("Cliente conectado")) << endl;

    //Sthreads.push_back( thread(atenderCliente, connectionfd, clientAddr, addrLen) );

    //despachante
    pthread_mutex_lock(&Mutex);
    /**********inicio regiao critica*********/
    while (fila.size() >= TAM_FILA)
        pthread_cond_wait(&prodCond, &Mutex);

    fila.push({connectionfd, clientAddr, addrLen}); //insere na fila
    pthread_cond_signal(&consCond);

    /************fim regiao critica***********/
    pthread_mutex_unlock(&Mutex);
}

void Server::atenderCliente(int connectionfd, sockaddr_in6 clientAddr, socklen_t addrLen){
    char addr_str[INET6_ADDRSTRLEN];
    getpeername(connectionfd, (struct sockaddr *)&clientAddr, &addrLen); //obtem o endereço do cliente conectado

    //converte o endereçoe porta de rede em string
    if (inet_ntop(AF_INET6, &clientAddr.sin6_addr, addr_str, INET6_ADDRSTRLEN) != NULL){
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
            break;
        case 0:
            perror(YEL("o cliente fechou a conexão antes que todos os dados fossem enviados"));
        default:
            cout << totalBytes << " Bytes recebidos --> ";
            printf("\"%s\"\n", buffer);
        }

        if (!strcmp(buffer, "bye") || !strcmp(buffer, "exit")){ //cliente quer parar a conexão
            continuar = false;
            strcpy(buffer, "Bye!");
            //shutdown(connectionfd, SHUT_RD); //fecha socket para recebimento de dados
        }
        else
            strcpy(buffer, "Sua mensagem foi recebida");

        totalBytes = send(connectionfd, buffer, strlen(buffer) + 1, MSG_NOSIGNAL);
        if (totalBytes == -1){
            cout << YEL("Falhou ao responder, cliente fechou a conexão") << endl;
            break;
        }
        else
            cout << CYN("resposta enviada") << endl;
    }

    //shutdown(connectionfd, SHUT_RDWR); //fecha socket para receber e enviar dados
    printf(BOLD(YEL("Fim da conexão\n")));
}


void Server::consumir(int num_thrd){
    Client_connection clientData;

    while (true){

        pthread_mutex_lock(&Mutex);
        /**********inicio regiao critica*********/
        while (fila.size() < 1)
            pthread_cond_wait(&consCond, &Mutex);
        
        
        cout << YEL("thread ") << num_thrd << endl;

        clientData = fila.front(); //pega o "cliente" para atende-lo
        fila.pop();
        pthread_cond_signal(&prodCond);

        /************fim regiao critica***********/
        pthread_mutex_unlock(&Mutex);

        Server::atenderCliente(clientData.connectionfd, clientData.clientAddr, clientData.addrLen);
        printf(BLU("\tThread %i liberada\n"), num_thrd);
    }
}


Server::~Server(){
    printf("Fechando servidor\n");
    Close();
}

void Server::Close(){
    printf("\t" BOLD("`") "->Finalizando threads 0 a %u", Sthreads.size()-1);
    pthread_cond_destroy(&consCond);
    pthread_cond_destroy(&prodCond);
    pthread_mutex_destroy(&Mutex);
    for (int i = 0; i < Sthreads.size(); i++){
        pthread_cancel(Sthreads[i].native_handle());
    }
    close(sockfd);
    close(connectionfd);
}

//handler para fechar sockets de todos os servers quando receber sinal ctrl+c
void Server::my_handler(int s){
    printf("\nCaught signal %d\n", s);
    printf(MAG("Fechando servidor\n"));
    for (int i = 0; i < instances.size(); i++){
        printf(BLU("\tFechando servidor %d\n"), i);            
        instances[i]->Close();
    }
    printf("\n");
    exit(1);
}