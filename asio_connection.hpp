#pragma once
#ifndef ___CONNECTION_HPP___
#define ___CONNECTION_HPP___

//#include <boost/archive/binary_iarchive.hpp>
//#include <boost/archive/binary_oarchive.hpp>
#include "eos/portable_iarchive.hpp"
#include "eos/portable_oarchive.hpp"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <array>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class Connection
{
private:
    enum {
        HEADER_LENGTH = 8,
    };

public:
    template<class T> using AsyncHandler = boost::function<void(const boost::system::error_code&, const boost::optional<T>&)>;
    template<class T> using AsyncHandlerPtr = std::shared_ptr<AsyncHandler<T>>;
    using HeaderBuffer = std::array<char, HEADER_LENGTH>;
    using BodyBuffer = std::vector<char>;

private:
    boost::asio::ip::tcp::socket socket_;

public:
    inline Connection(boost::asio::io_service& ioService);

    const boost::asio::ip::tcp::socket& getSocket() const { return socket_; }
    boost::asio::ip::tcp::socket& getSocket() { return socket_; }

    template<class T> inline void write(const T& t);
    template<class T> inline void read(T& t);

    template<class T> inline void asyncWrite(const T& t, const AsyncHandler<T>& orgHandler);
    template<class T> inline void asyncRead(const AsyncHandler<T>& orgHandler);
    
private:
    template<class T> inline void makeDataToWrite(const T& t, std::string& header, std::string& body);
    inline size_t determineBodyLength(const HeaderBuffer& header);
    template<class T> inline void extractData(T& t, const std::string& body);

    template<class T> inline void handleWrite(const boost::system::error_code& error, AsyncHandlerPtr<T> handler, std::shared_ptr<T> data, std::shared_ptr<std::string>, std::shared_ptr<std::string>);
    template<class T> inline void handleReadHeader(const boost::system::error_code& error, AsyncHandlerPtr<T> handler, std::shared_ptr<HeaderBuffer> headerBuffer);
    template<class T> inline void handleReadBody(const boost::system::error_code& error, AsyncHandlerPtr<T> handler, std::shared_ptr<BodyBuffer> bodyBuffer);
};

using ConnectionPtr = std::shared_ptr<Connection>;

///

Connection::Connection(boost::asio::io_service& ioService)
    : socket_(ioService)
{}

template<class T>
void Connection::write(const T& t)
{
    // make data to write
    std::string header, body;
    makeDataToWrite(t, header, body);

    // write
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(header));
    buffers.push_back(boost::asio::buffer(body));
    boost::asio::write(socket_, buffers);
}

template<class T>
void Connection::read(T& t)
{
    // read header
    HeaderBuffer header;
    boost::asio::read(socket_, boost::asio::buffer(header.data(), header.size()));
    
    // read body
    BodyBuffer body;
    body.resize(determineBodyLength(header));
    boost::asio::read(socket_, boost::asio::buffer(body));

    // extract data
    extractData(t, std::string(body.data(), body.size()));
}

template<class T>
void Connection::asyncWrite(const T& t, const AsyncHandler<T>& orgHandler)
{
    // make data to write
    auto header = std::make_shared<std::string>(), body = std::make_shared<std::string>();
    try{
        makeDataToWrite(t, *header, *body);
    }
    catch(boost::system::error_code &ex){
        socket_.get_io_service().post(boost::bind(orgHandler, ex, boost::none));
    }

    // copy data
    auto handler = std::make_shared<AsyncHandler<T>>(orgHandler);
    
    // write
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(*header));
    buffers.push_back(boost::asio::buffer(*body));
    boost::asio::async_write(socket_, buffers,
        boost::bind(&Connection::handleWrite<T>, this, boost::asio::placeholders::error, handler, nullptr, header, body));
}

template<class T>
void Connection::asyncRead(const AsyncHandler<T>& orgHandler)
{
    // prepare
    auto handler = std::make_shared<AsyncHandler<T>>(orgHandler);
    auto headerBuffer = std::make_shared<HeaderBuffer>();

    boost::asio::async_read(socket_, boost::asio::buffer(headerBuffer->data(), headerBuffer->size()),
        boost::bind(&Connection::handleReadHeader<T>, this, boost::asio::placeholders::error, handler, headerBuffer));
}

template<class T>
void Connection::makeDataToWrite(const T& t, std::string& header, std::string& body)
{
    // serialize
    std::ostringstream os;
    //boost::archive::binary_oarchive ar(os);
    eos::portable_oarchive ar(os);
    ar << t;
    body = os.str();

    // format the header
    os.str("");
    os << std::setw(HEADER_LENGTH) << std::hex << body.size();
    if(!os || os.str().size() != HEADER_LENGTH)
        throw boost::system::error_code(boost::asio::error::invalid_argument);
    header = os.str();
}

size_t Connection::determineBodyLength(const HeaderBuffer& header)
{
    std::istringstream is(std::string(header.data(), header.size()));
    std::size_t size = 0;
    if(!(is >> std::hex >> size))
        boost::system::error_code error(boost::asio::error::invalid_argument);
    return size;
}

template<class T>
void Connection::extractData(T& t, const std::string& body)
{
    std::istringstream is(body);
    //boost::archive::binary_iarchive ar(is);
    eos::portable_iarchive ar(is);
    ar >> t;
}

template<class T>
void Connection::handleWrite(const boost::system::error_code& error, AsyncHandlerPtr<T> handler, std::shared_ptr<T> data, std::shared_ptr<std::string>, std::shared_ptr<std::string>)
{
    (*handler)(error, boost::none);
}

template<class T>
void Connection::handleReadHeader(const boost::system::error_code& error, AsyncHandlerPtr<T> handler, std::shared_ptr<HeaderBuffer> headerBuffer)
{
    if(error){
        (*handler)(error, boost::none);
        return;
    }

    // determine length
    size_t size;
    try{
        size = determineBodyLength(*headerBuffer);
    }
    catch(boost::system::error_code& ex){
        // header isn't valid, inform the caller
        (*handler)(ex, boost::none);
        return;
    }

    // start to receive
    auto bodyBuffer = std::make_shared<BodyBuffer>(size);
    boost::asio::async_read(socket_, boost::asio::buffer(*bodyBuffer),
        boost::bind(&Connection::handleReadBody<T>, this, boost::asio::placeholders::error, handler, bodyBuffer));
}

template<class T>
void Connection::handleReadBody(const boost::system::error_code& error, AsyncHandlerPtr<T> handler, std::shared_ptr<BodyBuffer> bodyBuffer)
{
    if(error && error != boost::asio::error::eof){
        (*handler)(error, boost::none);
        return;
    }

    // extract data
    T t;
    try{
        std::string data(bodyBuffer->data(), bodyBuffer->size());
        extractData(t, data);
    }
    catch(std::exception& ex){
        // unable to decode data
        (*handler)(error, boost::none);
        return;
    }

    (*handler)(error, t);
}

#endif

