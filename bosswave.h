
#ifndef __BOSSWAVE_H__
#define __BOSSWAVE_H__

#define CMD_HELLO             "helo"
#define CMD_PUBLISH           "publ"
#define CMD_SUBSCRIBE         "subs"
#define CMD_PERSIST           "pers"
#define CMD_LIST              "list"
#define CMD_QUERY             "quer"
#define CMD_TAP_SUBSCRIBE     "tsub"
#define CMD_TAP_QUERY         "tque"
#define CMD_PUT_DOT           "putd"
#define CMD_PUT_ENTITY        "pute"
#define CMD_PUT_CHAIN         "putc"
#define CMD_MAKE_DOT          "makd"
#define CMD_MAKE_ENTITY       "make"
#define CMD_MAKE_CHAIN        "makc"
#define CMD_BUILD_CHAIN       "bldc"
#define CMD_ADD_PREF_DOT      "adpd"
#define CMD_ADD_PREF_CHAIN    "adpc"
#define CMD_DEL_PREF_DOT      "dlpd"
#define CMD_DEL_PREF_CHAIN    "dlpc"
#define CMD_SET_ENTITY        "sete"
#define CMD_RESPONSE          "resp"
#define CMD_RESULT            "rslt"

#include <stdio.h>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>
#include <memory>
#include <mutex>

namespace bw
{
  typedef struct
  {
    std::string key;
    std::string value;
  } kv_t;
  typedef std::function<void(std::string)> stat_cb_t;
  class PayloadObject
  {
  public:
    PayloadObject(int ponum, size_t size);
    PayloadObject(int ponum, std::string contents);
    ~PayloadObject();
    size_t GetLength();
    char* GetContentsChar();
    std::string GetContentsString();
    int GetPONum();
  private:
    char* contents;
    int ponum;
    size_t len;
  };
  class BosswaveClient;
  class Frame
  {
    friend class BosswaveClient;
  public:
    Frame(int seqno, std::string type);
    void AddKV(std::string key, char* value, size_t len);
    void AddKV(std::string key, std::string value);
    void AddPO(PayloadObject* po);

    int GetSeqNo();
    int GetNumKV();
    int GetNumPO();
    std::string GetType();
    std::string GetKV(std::string key);
    PayloadObject* GetPO(int idx);
  private:
    //BosswaveClient *bwc;
    std::map<std::string, std::string> kvs;
    std::string type;
    std::vector<PayloadObject*> pos;
    int seqno;
  };

  typedef std::function<void(bool, std::string)> status_cb;
  typedef std::function<void(std::shared_ptr<Frame>)> result_cb;
  typedef std::function<void(std::string, bool, std::string)> sete_cb;
  int fromdotform(std::string);
  class BosswaveClient
  {
  public:
    BosswaveClient(std::string target, int port);
    void SetEntity(std::string entityfile,
        std::function<void(std::string, bool, std::string)> ondone);
    void Subscribe(std::string uri,
        std::function<void(bool, std::string)> statuscb,
        std::function<void(std::shared_ptr<Frame>)> resultcb);
    void Publish(std::string uri, std::vector<PayloadObject*> &pos,
        std::function<void(bool, std::string)> statuscb);
  private:
    void readloop();
    void readframe();
    void unregisterCB(int seqno);
    void sendFrame(std::shared_ptr<Frame> f, std::function<void(std::shared_ptr<Frame>)> cb);
    std::shared_ptr<Frame> NewFrame(std::string command);
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket sock;
    boost::thread *rthread;
    boost::asio::streambuf rbuf;
    std::mutex sendlock;
    std::mutex seqlock;
    std::map<int, std::function<void(std::shared_ptr<Frame>)> > seqmap;
    boost::atomic<int> seqno;
  };




}
#endif
