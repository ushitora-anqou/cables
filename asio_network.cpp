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
    : ioService_(ioService),
      acceptor_(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      conn_(std::make_shared<Connection>(ioService)),
      canSendToNext_(false)
{}

void AsioNetworkRecvUnit::startRead()
{
    conn_->asyncRead<WaveData>(boost::bind(&AsioNetworkRecvUnit::handleRecvWaveData, this, _1, _2));
}

void AsioNetworkRecvUnit::startImpl()
{
    ioService_.post([this]() {
        acceptor_.async_accept(conn_->getSocket(),[this](const boost::system::error_code& error) {
            if(error){
                std::cout << "ASYNC_ACCEPT_ERROR: " << error.message() << std::endl;
                return;
            }

            canSendToNext_ = true;
            startRead();
        });
    });
}

void AsioNetworkRecvUnit::stopImpl()
{
    ioService_.post([this]() {
        acceptor_.close();
        conn_->getSocket().close();
        canSendToNext_ = false;
    });
}

void AsioNetworkRecvUnit::handleRecvWaveData(const boost::system::error_code& error, const boost::optional<WaveData>& data)
{
    if(error || !data){
        std::cout << "ASYNC_RECV_ERROR: " << error.message() << std::endl;
        canSendToNext_ = false;
        return;
    }

    auto& wave = *data;
    send(wave.data);
    startRead();
}

///

AsioNetworkSendUnit::AsioNetworkSendUnit(boost::asio::io_service& ioService, const unsigned short port, const std::string& ipaddr)
    : ioService_(ioService), conn_(std::make_shared<Connection>(ioService)), port_(port), ipaddr_(ipaddr)
{}

void AsioNetworkSendUnit::startSend()
{
    conn_->asyncWrite<WaveData>(*waveQue_.front(),
        [this](const boost::system::error_code& error, const boost::optional<WaveData>&) {
            waveQue_.pop();
            if(error){
                std::cout << "ASYNC_WRITE_ERROR: " << error.message() << std::endl;
                return;
            }
            if(!waveQue_.empty())   startSend();
        }
    );
}

void AsioNetworkSendUnit::startImpl()
{
    ioService_.post([this]() {
        conn_->getSocket().async_connect(
            boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ipaddr_), port_),
            [this](const boost::system::error_code& error) {
                if(error){
                    std::cout << "ASYNC_CONNECT_ERROR: " << error.message() << std::endl;
                    return;
                }
            }
        );
    });
}

void AsioNetworkSendUnit::stopImpl()
{
    ioService_.post([this]() { conn_->getSocket().close(); });
}

void AsioNetworkSendUnit::inputImpl(const PCMWave& wave)
{
    std::shared_ptr<WaveData> data = std::make_shared<WaveData>(wave);
    ioService_.post([this, data]() {
        if(!conn_->getSocket().is_open()) return;
        bool isProcessing = !waveQue_.empty();
        waveQue_.push(data);
        if(!isProcessing)   startSend();
    });
}

/*
// Pool ver
void AsioNetworkSendUnit::inputImpl(const PCMWave& wave)
{
    std::shared_ptr<PCMWave> data = std::make_shared<PCMWave>(wave);
    ioService_.post([this, data]() {
        currentData_->data.at(currentDataIndex_++) = data;
        if(currentDataIndex_ != currentData_->data.size())  return;
        bool isProcessing = !waveQue_.empty();
        waveQue_.push(currentData_);
        currentData_ = std::make_shared<WaveData>();
        currentDataIndex_ = 0;
        if(!isProcessing)   startSend();
    });
}
*/
