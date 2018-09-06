#include "Server.h"


class SubServer;

class SubServer: public Server{

 
  protected:
    void atenderServer(int connectionfd, int threadNumber);
    
    static void my_signal_handler(int s);
    static SubServer *instance;

  public:
    SubServer(int porta);
    ~SubServer();
    virtual void Start();
    virtual void Listen(size_t listen_buffer_size);
    virtual void Accept();
    virtual void Close();

};
