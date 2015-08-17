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

class AsioNetworkSystem
{
private:
    boost::asio::io_service ioService_;
    std::unique_ptr<boost::thread> ioServer_;
    std::unique_ptr<boost::asio::io_service::work> work_;

public:
    AsioNetworkSystem();
    ~AsioNetworkSystem();

    boost::asio::io_service& getSystemInfo() { return ioService_; }
};

// recv, sendのUnitは、コンストラクタで接続・情報交換
// startが呼ばれるまでwaveデータの送受信はしない

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
    bool willContinue;
    PCMWave data;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & willContinue & data;
    }
};

class AsioNetworkRecvUnit : public Unit
{
private:
    boost::asio::ip::tcp::acceptor acceptor_;
    ConnectionPtr conn_;
    bool canSendToNext_;

private:
    void startRead();

public:
    AsioNetworkRecvUnit(boost::asio::io_service& ioService, const unsigned short port);

    bool canSendToNext() { return canSendToNext_; }

    void startImpl();
    void stopImpl();

    void handleRecvWaveData(const boost::system::error_code& error, const boost::optional<WaveData>& data);
};

#endif
