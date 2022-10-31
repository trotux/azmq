/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of azmq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZMQ_DETAIL_RECEIVE_OP_HPP_
#define AZMQ_DETAIL_RECEIVE_OP_HPP_

#include "../error.hpp"
#include "../message.hpp"
#include "socket_ops.hpp"
#include "reactor_op.hpp"

#include <boost/asio/io_service.hpp>

#include <zmq.h>

#include <iterator>

namespace azmq {
namespace detail {
template<typename MutableBufferSequence>
class receive_buffer_op_base : public reactor_op {
public:
    receive_buffer_op_base(MutableBufferSequence const& buffers, flags_type flags)
        : buffers_(buffers)
        , flags_(flags)
        { }

    virtual bool do_perform(socket_type& socket) override {
        ec_ = boost::system::error_code();
        bytes_transferred_ += socket_ops::receive(buffers_, socket, flags_ | ZMQ_DONTWAIT, ec_);
        if (ec_)
            return !try_again();
        return true;
    }

protected:
    bool more() const {
        return ec_ == boost::system::errc::no_buffer_space && bytes_transferred_;
    }

private:
    MutableBufferSequence buffers_;
    flags_type flags_;
};

template<typename MutableBufferSequence,
         typename Handler>
class receive_buffer_op : public receive_buffer_op_base<MutableBufferSequence> {
public:
    receive_buffer_op(MutableBufferSequence const& buffers,
                      Handler handler,
                      socket_ops::flags_type flags)
        : receive_buffer_op_base<MutableBufferSequence>(buffers, flags)
        , handler_(std::move(handler))
        { }

    virtual void do_complete() override {
        handler_(this->ec_, this->bytes_transferred_);
    }

private:
    Handler handler_;
};

template<typename MutableBufferSequence,
         typename Handler>
class receive_more_buffer_op : public receive_buffer_op_base<MutableBufferSequence> {
public:
    receive_more_buffer_op(MutableBufferSequence const& buffers,
                           Handler handler,
                           socket_ops::flags_type flags)
        : receive_buffer_op_base<MutableBufferSequence>(buffers, flags)
        , handler_(std::move(handler))
        { }

    virtual void do_complete() override {
        handler_(this->ec_, std::make_pair(this->bytes_transferred_, this->more()));
    }

private:
    Handler handler_;
};

template<typename T, typename Handler>
class receive_op : public reactor_op {
public:
    receive_op(Handler handler,
               socket_ops::flags_type flags)
        : flags_(flags)
        , handler_(std::move(handler))
        { }

    virtual bool do_perform(socket_type& socket) override {
        ec_ = boost::system::error_code();
        bytes_transferred_ = socket_ops::receive(msg_, socket, flags_ | ZMQ_DONTWAIT, ec_);
        if (ec_)
            return !try_again();
        return true;
    }

    virtual void do_complete() override {
        handler_(this->ec_, this->msg_, this->bytes_transferred_);
    }

private:
    flags_type flags_;
    Handler handler_;
    T msg_;
};

} // namespace detail
} // namespace azmq
#endif // AZMQ_DETAIL_RECEIVE_OP_HPP_


