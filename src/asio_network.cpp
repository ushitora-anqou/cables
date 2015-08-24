#include "asio_network.hpp"
#include "helper.hpp"

AsioNetworkBase::AsioNetworkBase()
{
    work_ = make_unique<boost::asio::io_service::work>(ioService_);
    ioServer_ = make_unique<boost::thread>([this]() { ioService_.run(); });
}

AsioNetworkBase::~AsioNetworkBase()
{
    work_.reset();
    ioServer_->join();
}

std::unique_ptr<boost::asio::ip::tcp::acceptor> AsioNetworkBase::createAcceptor(unsigned short port)
{
    return make_unique<boost::asio::ip::tcp::acceptor>(
        ioService_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
}

ConnectionPtr AsioNetworkBase::createConnection()
{
    return std::make_shared<Connection>(ioService_);
}


///

AsioNetworkRecvUnit::AsioNetworkRecvUnit(unsigned short port)
    : canSendToNext_(false)
{
    acceptor_ = createAcceptor(port);
    conn_ = createConnection();
}

void AsioNetworkRecvUnit::startRead()
{
    conn_->asyncRead<WaveData>(boost::bind(&AsioNetworkRecvUnit::handleRecvWaveData, this, _1, _2));
}

void AsioNetworkRecvUnit::startImpl()
{
    postProc([this]() {
        acceptor_->async_accept(conn_->getSocket(),[this](const boost::system::error_code& error) {
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
    postProc([this]() {
        acceptor_->close();
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

AsioNetworkSendUnit::AsioNetworkSendUnit(const unsigned short port, const std::string& ipaddr)
    : port_(port), ipaddr_(ipaddr)
{
    conn_ = createConnection();
}

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
    postProc([this]() {
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
    postProc([this]() { conn_->getSocket().close(); });
}

void AsioNetworkSendUnit::inputImpl(const PCMWave& wave)
{
    std::shared_ptr<WaveData> data = std::make_shared<WaveData>(wave);
    postProc([this, data]() {
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
    postProc([this, data]() {
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
