#include "bosswave.h"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <cstring>

using namespace boost::asio::ip;
using namespace bw;

std::shared_ptr<Frame> BosswaveClient::NewFrame(std::string command)
{
  int newseqno = ++seqno;
  return std::shared_ptr<Frame>(new Frame(newseqno, command));
}
BosswaveClient::BosswaveClient(std::string target, int port)
  : sock(io_service), rbuf(1*1024*1024)
{
  tcp::endpoint ep = tcp::endpoint(address::from_string(target), port);
  boost::system::error_code error;
  sock.connect(ep, error);
  if (error) {
    throw boost::system::system_error(error); // Some other error.
  }
  rthread = new boost::thread(boost::bind(&BosswaveClient::readloop, this));

}
void BosswaveClient::unregisterCB(int seqno)
{
  seqlock.lock();
  auto cb = seqmap.find(seqno);
  if (cb == seqmap.end()) {
    throw std::runtime_error("bad seqno");
  }
  seqmap.erase(cb);
  seqlock.unlock();
}
void BosswaveClient::readframe()
{
  boost::system::error_code error;

  size_t n = boost::asio::read_until(sock, rbuf, "\n", error);
  std::istream is(&rbuf);
  if (n != 27) {
    throw std::runtime_error("bad frame");
  }
  std::string cmd;
  int length;
  int seqno;
  is >> cmd;
  is >> length;
  is >> seqno;
  rbuf.consume(1); // \n
  std::shared_ptr<Frame> frame = std::shared_ptr<Frame>(new Frame(seqno, cmd));
  while(1)
  {
    size_t n = boost::asio::read_until(sock, rbuf, "\n", error);
    if (error)
      throw boost::system::system_error(error); // Some other error.
    std::string type;
    std::string key;
    int len;
    is >> type;
    if (type == "end") {
      rbuf.consume(1);
      if (cmd == "helo") {
        std::cout << "connected to router " << frame->GetKV("version") << std::endl;
        return;
      }
      seqlock.lock();
      auto cb = seqmap.find(frame->GetSeqNo());
      if (cb == seqmap.end()) {
        throw std::runtime_error("bad seqno");
      }
      std::function<void(std::shared_ptr<Frame>)> cbf = cb->second;
      seqlock.unlock();
      cbf(frame);
      return;
    } else if (type == "kv") {
      std::string key;
      int len;
      is >> key;
      is >> len;
      rbuf.consume(1); // \n
      char contents[len];
      if (rbuf.size() < len) {
        boost::asio::read(sock, rbuf, boost::asio::transfer_at_least(len-rbuf.size()), error);
        if (error) throw boost::system::system_error(error); // Some other error.
      }
      rbuf.sgetn(contents, len);
      rbuf.consume(1);
      frame->AddKV(key, std::string(contents, len));
    } else if (type == "po") {
      std::string ponums;
      int len;
      int ponum;
      is >> ponums;
      is >> len;
      rbuf.consume(1);
      std::string intform = ponums.substr(ponums.find(':'));
      ponum = boost::lexical_cast<int>(intform);
      PayloadObject *po = new PayloadObject(ponum, len);
      if (rbuf.size() < len) {
        boost::asio::read(sock, rbuf, boost::asio::transfer_at_least(len-rbuf.size()), error);
        if (error) throw boost::system::system_error(error); // Some other error.
      }
      rbuf.sgetn(po->GetContentsChar(), len);
      frame->AddPO(po);
      rbuf.consume(1);
    } else if (type == "ro") {
      int ronum;
      int len;
      is >> ronum;
      is >> len;
      rbuf.consume(1);
      if (rbuf.size() < len) {
        boost::asio::read(sock, rbuf, boost::asio::transfer_at_least(len-rbuf.size()), error);
        if (error) throw boost::system::system_error(error); // Some other error.
      }
      //TODO store this
      rbuf.consume(len);
      rbuf.consume(1);
    }
  }
}
int fromdotform(std::string v) {
  int a, b, c, d;
  std::sscanf(v.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);
  return (a<<24) + (b<<16) + (c<<8) + d;
}
void BosswaveClient::readloop()
{
  for (;;) {
    readframe();
  }
}

PayloadObject::PayloadObject(int ponum, size_t size)
    : ponum(ponum),
      contents(new char[size]),
      len(size)
{

}
PayloadObject::PayloadObject(int ponum, std::string val)
  : ponum(ponum),
    contents(new char[val.size()]),
    len(val.size())
{
  std::memcpy(contents, val.data(), val.size());
}
PayloadObject::~PayloadObject()
{
    delete [] contents;
}
char* PayloadObject::GetContentsChar()
{
    return contents;
}
size_t PayloadObject::GetLength()
{
    return len;
}
std::string PayloadObject::GetContentsString()
{
    return std::string(contents, len);
}

Frame::Frame(int seqno, std::string type)
  : type(type),
    seqno(seqno)
{
}

std::string Frame::GetKV(std::string key)
{
  std::map<std::string, std::string>::iterator kv = kvs.find(key);
  if (kv == kvs.end()) {
    return std::string("");
  }
  return kv->second;
}

int Frame::GetSeqNo()
{
  return seqno;
}
int PayloadObject::GetPONum()
{
  return ponum;
}
std::string Frame::GetType()
{
  return type;
}
void Frame::AddKV(std::string key, std::string value)
{
  kvs[key] = value;
}
void Frame::AddPO(PayloadObject* po)
{
  pos.push_back(po);
}
void BosswaveClient::Subscribe(std::string uri,
    std::function<void(bool, std::string)> statuscb,
    std::function<void(std::shared_ptr<Frame>)> resultcb)
{
  std::shared_ptr<Frame> f = NewFrame(CMD_SUBSCRIBE);
  f->AddKV("elaborate_pac", "full");
  f->AddKV("unpack", "true");
  f->AddKV("autochain", "true");
  f->AddKV("uri", uri);
  sendFrame(f, std::function<void(std::shared_ptr<Frame>)>([=](std::shared_ptr<Frame> r){
    std::string status = r->GetKV("status");
    if (status != "")
    {
      //First frame
      if (status != "okay") {
        statuscb(false, r->GetKV("reason"));
      } else {
        statuscb(true, "");
      }
    } else {
      //Message frame
      resultcb(r);
    }
  }));
}
void BosswaveClient::Publish(std::string uri, std::vector<PayloadObject*> &pos,
    std::function<void(bool, std::string)> statuscb)
{
  std::shared_ptr<Frame> f = NewFrame(CMD_PUBLISH);
  f->AddKV("elaborate_pac", "full");
  f->AddKV("autochain", "true");
  f->AddKV("uri", uri);
  for (auto it = pos.begin(); it < pos.end(); it++)
  {
    f->AddPO(*it);
  }
  sendFrame(f, std::function<void(std::shared_ptr<Frame>)>([=](std::shared_ptr<Frame> r){
    std::string status = r->GetKV("status");
    if (status == "okay") {
      statuscb(true, "");
    } else {
      statuscb(false, r->GetKV("reason"));
    }
    unregisterCB(r->GetSeqNo());
  }));
}
void BosswaveClient::SetEntity(std::string entityfile, std::function<void(std::string, bool, std::string)> ondone)
{
  std::ifstream efile;
  efile.open(entityfile, std::ios::in | std::ios::binary);
  std::string contents((std::istreambuf_iterator<char>(efile)),
                        std::istreambuf_iterator<char>());
  std::cout << "entity keyfile len: " << contents.size() << std::endl;
  std::shared_ptr<Frame> f = NewFrame(CMD_SET_ENTITY);
  f->AddPO(new PayloadObject(0x01000102, contents.substr(1)));
  sendFrame(f, std::function<void(std::shared_ptr<Frame>)>([=](std::shared_ptr<Frame> r){
    std::string status = r->GetKV("status");
    if (status != "okay") {
      ondone("", false, r->GetKV("reason"));
    } else {
      ondone(r->GetKV("vk"), true, "");
    }
  }));
}
void BosswaveClient::sendFrame(std::shared_ptr<Frame> f, std::function<void(std::shared_ptr<Frame>)> cb)
{
  seqlock.lock();
  seqmap[f->GetSeqNo()] = cb;
  seqlock.unlock();
  sendlock.lock();
  boost::asio::streambuf wbuf;
  std::ostream os(&wbuf);
  os << boost::format("%4s %010d %010d\n") % f->GetType() % 0 % f->GetSeqNo();
  for (auto it = f->kvs.begin(); it != f->kvs.end(); it++)
  {
    std::string key = it->first;
    std::string value = it->second;
    os << "kv " << key << " " << value.size() << "\n" << value << "\n";
    boost::asio::write(sock, wbuf);
  }
  for (auto it = f->pos.begin(); it != f->pos.end(); it++)
  {
    PayloadObject *po = *it;
    os << "po :" << po->GetPONum() << " " << po->GetLength() << "\n";
    os.write(po->GetContentsChar(), po->GetLength());
    os << "\n";
    boost::asio::write(sock, wbuf);
  }
  os << "end\n";
  boost::asio::write(sock, wbuf);
  sendlock.unlock();
}
