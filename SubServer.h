#include "Server.h"


class SubServer;

class SubServer: public Server{

 
  protected:
    void atenderServer(int connectionfd);

  public:
    static void my_handler(int s);
    SubServer(int porta);
    ~SubServer();
    void Start();
    void Accept();
    void Close();

};
