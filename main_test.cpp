
#include "bosswave.h"

int main() {
  bw::BosswaveClient *bwc = new bw::BosswaveClient("127.0.0.1", 28589);
  bwc->SetEntity("entity.key",
      bw::sete_cb([=](std::string us, bool ok, std::string msg){
        std::cout << "sete status: " << ok << " msg: " << msg << std::endl;
        std::cout << "us: " << us << std::endl;
        if (ok) {
          auto pov = std::vector<bw::PayloadObject*>();
          pov.push_back(new bw::PayloadObject(5, "hello world"));
          bwc->Publish("castle.bw2.io/cxx/foobar", pov,
            bw::status_cb([](bool ok, std::string msg){
              std::cout << "publish status: " << ok << " msg: " << msg << std::endl;
            }));
        }
    }));
  while(1);
  /*
  while(1) {


    size_t len = socket.read_some(boost::asio::buffer(buf), error);
    if (error == boost::asio::error::eof)
      break; // Connection closed cleanly by peer.
    else if (error)
      throw boost::system::system_error(error); // Some other error.

    std::cout.write(buf.data(), len);
  }
  */

}
