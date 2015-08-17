#include "asio_network.hpp"
#include "helper.hpp"

AsioNetworkSystem::AsioNetworkSystem()
{
    work_ = make_unique<boost::asio::io_service::work>(ioService_);
    ioServer_ = make_unique<boost::thread>([this]() { ioService_.run(); });
}

AsioNetworkSystem::~AsioNetworkSystem()
{
    work_.reset();
    ioServer_->join();
}

///

AsioNetworkRecvUnit::AsioNetworkRecvUnit(boost::asio::io_service& ioService, const unsigned short port)
    : acceptor_(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      conn_(std::make_shared<Connection>(ioService)),
      canSendToNext_(false)
{}

void AsioNetworkRecvUnit::startRead()
{
    conn_->asyncRead<WaveData>(boost::bind(&AsioNetworkRecvUnit::handleRecvWaveData, this, _1, _2));
}

void AsioNetworkRecvUnit::startImpl()
{
    auto& conn = conn_;
    acceptor_.async_accept(conn_->getSocket(),[this](const boost::system::error_code& error) {
        if(error){
            std::cout << "ASYNC_ACCEPT_ERROR: " << error.message() << std::endl;
            return;
        }

        canSendToNext_ = true;
        startRead();
    });
}

void AsioNetworkRecvUnit::stopImpl()
{
    acceptor_.close();
    conn_->getSocket().close();
    canSendToNext_ = false;
}

void AsioNetworkRecvUnit::handleRecvWaveData(const boost::system::error_code& error, const boost::optional<WaveData>& data)
{
    if(error || !data){
        std::cout << "ASYNC_ACCEPT_ERROR: " << error.message() << std::endl;
        canSendToNext_ = false;
        return;
    }

    auto& wave = *data;
    send(wave.data);
    canSendToNext_ = wave.willContinue;
    startRead();
}
