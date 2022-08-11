// TcpClient destructs in a different thread.

#include "convey/base/Logging.h"
#include "convey/net/EventLoopThread.h"
#include "convey/net/TcpClient.h"

using namespace convey;
using namespace convey::net;

int main(int argc, char *argv[])
{
  Logger::setLogLevel(Logger::DEBUG);

  EventLoopThread loopThread;
  {
    InetAddress serverAddr("127.0.0.1", 1234);  // should succeed
    TcpClient client(loopThread.startLoop(), serverAddr, "TcpClient");
    client.connect();
    CurrentThread::sleepUsec(500 * 1000);  // wait for connect
    client.disconnect();
  }

  CurrentThread::sleepUsec(1000 * 1000);
}
