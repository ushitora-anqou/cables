#pragma once
#ifndef ___ASIO_NETWORK_HPP___
#define ___ASIO_NETWORK_HPP___

// ライブラリ非依存に書こうとしたが挫折
// どうせC++だとboost::asio以外にまともなネットワークライブラリなんてないし……
// ということでboost::asioのみ対応、Adapter書ける人求む

#include "asio_connection.hpp"
#include "socket.hpp"
#include "pcmwave.hpp"
#include <boost/serialization/array.hpp>
#include <boost/thread.hpp>
#include <deque>

namespace boost {
    namespace serialization {
        template <class Archive>
        void serialize(Archive& ar, PCMWave::Sample& data, const unsigned int version) 
        {
            ar & data.left & data.right;
        }

        template <class Archive>
        void serialize(Archive& ar, PCMWave& data, const unsigned int version) 
        {
            ar & data.get();
        }
    }
}

struct WaveData
{
    PCMWave data;

    WaveData(){}
    WaveData(const PCMWave& data_)
        : data(data_)
    {}

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & data;
    }
};

class AsioNetworkBase
{
private:
    boost::asio::io_service ioService_;
    std::unique_ptr<boost::asio::io_service::work> work_;
    std::unique_ptr<boost::thread> ioServer_;

public:
    AsioNetworkBase();
    virtual ~AsioNetworkBase();

    void kill();
    std::unique_ptr<boost::asio::ip::tcp::acceptor> createAcceptor(unsigned short port);
    ConnectionPtr createConnection();

    template<class Proc> void postProc(Proc proc)
    {
        ioService_.post(proc);
    }
};


class AsioNetworkRecvOutUnit : public Unit, private AsioNetworkBase
{
private:
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    ConnectionPtr conn_;
    bool hasConnected_;

private:
    void startAccept();
    void startRead();

public:
    AsioNetworkRecvOutUnit(unsigned short port);
    ~AsioNetworkRecvOutUnit();

    void startImpl();
    void stopImpl();

    void handleRecvWaveData(const boost::system::error_code& error, const boost::optional<WaveData>& data);
};

class AsioNetworkSendInUnit : public Unit, private AsioNetworkBase
{
private:
    ConnectionPtr conn_;
    unsigned short port_;
    std::string ipaddr_;
    std::deque<std::shared_ptr<WaveData>> waveQue_;
    bool hasConnected_;

private:
    void startConnect();
    void startSend();
    void closeConnect();

public:
    AsioNetworkSendInUnit(const unsigned short port, const std::string& ipaddr);
    ~AsioNetworkSendInUnit();

    void startImpl();
    void stopImpl();
    void inputImpl(const PCMWave& wave);
};

#endif
