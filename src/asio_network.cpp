#include "asio_network.hpp"
#include "helper.hpp"
#include "error.hpp"

AsioNetworkBase::AsioNetworkBase()
{
    work_ = make_unique<boost::asio::io_service::work>(ioService_);
    ioServer_ = make_unique<boost::thread>([this]() {
        try{
            ioService_.run();
        }
        catch(std::exception& ex){
            ZARU_CHECK(ex.what());
        }
        catch(...){
            ZARU_CHECK("fatal error");
        }
    });
}

AsioNetworkBase::~AsioNetworkBase()
{
    kill();
}

void AsioNetworkBase::kill()
{
    if(work_){
        work_.reset();
        ioServer_->join();
    }
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

AsioNetworkRecvOutUnit::AsioNetworkRecvOutUnit(unsigned short port)
    : hasConnected_(false)
{
    acceptor_ = createAcceptor(port);
}

AsioNetworkRecvOutUnit::~AsioNetworkRecvOutUnit()
{
    kill();
}

void AsioNetworkRecvOutUnit::startAccept()
{
    hasConnected_ = false;
    conn_ = createConnection();
    acceptor_->async_accept(conn_->getSocket(),[this](const boost::system::error_code& error) {
        if(error){
            std::cout << "ASYNC_ACCEPT_ERROR: " << error.message() << std::endl;
            return;
        }

        hasConnected_ = true;
        setSocketStatus(true);
        startRead();
    });
}

void AsioNetworkRecvOutUnit::startRead()
{
    conn_->asyncRead<WaveData>(boost::bind(&AsioNetworkRecvOutUnit::handleRecvWaveData, this, _1, _2));
}

void AsioNetworkRecvOutUnit::startImpl()
{
    setSocketStatus(false);
    postProc([this]() { startAccept(); });
}

void AsioNetworkRecvOutUnit::stopImpl()
{
    postProc([this]() {
        acceptor_->cancel();
        conn_.reset();
        hasConnected_ = false;
        setSocketStatus(false);
    });
}

void AsioNetworkRecvOutUnit::handleRecvWaveData(const boost::system::error_code& error, const boost::optional<WaveData>& data)
{
    if(error || !data){
        std::cout << "ASYNC_RECV_ERROR: " << error.message() << std::endl;
        setSocketStatus(false);
        hasConnected_ = false;
        if(error == boost::asio::error::eof)    startAccept();
        return;
    }

    auto& wave = *data;
    send(wave.data);
    if(hasConnected_)   startRead();
}

///

AsioNetworkSendInUnit::AsioNetworkSendInUnit(const unsigned short port, const std::string& ipaddr)
    : port_(port), ipaddr_(ipaddr), hasConnected_(false)
{}

AsioNetworkSendInUnit::~AsioNetworkSendInUnit()
{
    kill();
}

void AsioNetworkSendInUnit::startConnect()
{
    conn_ = createConnection();
    hasConnected_ = false;
    conn_->getSocket().async_connect(
        boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ipaddr_), port_),
        [this](const boost::system::error_code& error) {
            if(error){
                std::cout << "ASYNC_CONNECT_ERROR: " << error.message() << std::endl;
                return;
            }
            hasConnected_ = true;
        }
    );
}

void AsioNetworkSendInUnit::startSend()
{
    conn_->asyncWrite<WaveData>(*waveQue_.front(),
        [this](const boost::system::error_code& error, const boost::optional<WaveData>&) {
            waveQue_.pop_front();
            if(error){
                std::cout << "ASYNC_WRITE_ERROR: " << error.message() << std::endl;
                closeConnect();
                return;
            }
            if(!waveQue_.empty())   startSend();
        }
    );
}

void AsioNetworkSendInUnit::closeConnect()
{
    conn_.reset();
    hasConnected_ = false;
}

void AsioNetworkSendInUnit::startImpl()
{
    postProc([this]() { startConnect(); });
}

void AsioNetworkSendInUnit::stopImpl()
{
    postProc([this]() {
        closeConnect();
        waveQue_.clear();
    });
}

void AsioNetworkSendInUnit::inputImpl(const PCMWave& wave)
{
    std::shared_ptr<WaveData> data = std::make_shared<WaveData>(wave);
    postProc([this, data]() {
        if(!hasConnected_) return;
        bool isProcessing = !waveQue_.empty();
        waveQue_.push_back(data);
        if(!isProcessing)   startSend();
    });
}

/*
// Pool ver
void AsioNetworkSendInUnit::inputImpl(const PCMWave& wave)
{
    std::shared_ptr<PCMWave> data = std::make_shared<PCMWave>(wave);
    postProc([this, data]() {
        currentData_->data.at(currentDataIndex_++) = data;
        if(currentDataIndex_ != currentData_->data.size())  return;
        bool isProcessing = !waveQue_.empty();
        waveQue_.push_back(currentData_);
        currentData_ = std::make_shared<WaveData>();
        currentDataIndex_ = 0;
        if(!isProcessing)   startSend();
    });
}
*/
