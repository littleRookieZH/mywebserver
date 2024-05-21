/**
 * @file socket_stream.h
 * @author your name (you@domain.com)
 * @brief Socket流式接口封装
 * socket.h中的封装针对的是原始的c API的 -- 将其封装成类
 * socket_stream.h 针对的是业务级的接口封装  -- 一般不会直接使用原始的api
 * @version 0.1
 * @date 2024-05-21
 * 
 * 
 */
#ifndef __WEBS_SOCKET_STREAM_H__
#define __WEBS_SOCKET_STREAM_H__

#include "stream.h"
#include "../net_module/socket.h"

namespace webs {
class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;
    /**
     * @brief Construct a new Socket Stream object
     * 
     * @param sock Socket类
     * @param owner 是否完全控制  -- 决定当前对象是否具有关闭 socketfd的权限
     */
    SocketStream(Socket::ptr sock, bool owner = true);

    /**
     * @brief Destroy the Socket Stream object
     * 如果m_owner=true,则close
     */
    ~SocketStream();

    /**
     * @brief 读取数据
     * 
     * @param buf 待接收数据的内存
     * @param length 待接收数据的内存长度
     * @return int 
     *      @retval >0 返回实际接收到的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    virtual int read(void *buf, size_t length) override;

    /**
     * @brief 读取数据
     * 
     * @param ba 接收数据的ByteArray
     * @param length 待接收数据的内存长度
     * @return int 
     *      @retval >0 实际接收的内存长度
     *      @retval =0 socket远端被关闭
     *      @retval <0 socket错误
     */
    virtual int read(ByteArray::ptr ba, size_t length) override;

    /**
     * @brief 写入数据
     * 
     * @param buf 待发送数据的内存
     * @param length 待发送数据的内存长度
     * @return int 
     *      @retval >0 返回实际接收到的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    virtual int write(const void *buf, size_t length) override;

    /**
     * @brief 写入数据
     * 
     * @param ba 待发送数据的ByteArray
     * @param length 待发送数据的内存长度
     * @return int 
     *      @retval >0 返回实际接收到的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    virtual int write(ByteArray::ptr ba, size_t length) override;

    /**
     * @brief 关闭socket
     * 
     */
    virtual void close() override;

    /**
     * @brief Get the Socket object
     * 
     * @return Socket::ptr 
     */
    Socket::ptr getSocket() const {
        return m_socket;
    }

    /**
     * @brief 返回是否连接
     * 
     * @return true 
     * @return false 
     */
    bool isConnected() const;

    /**
     * @brief 获取远端地址
     * 
     * @return Address::ptr 
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本地地址
     * 
     * @return Address::ptr 
     */
    Address::ptr getLocalAddress();

    /**
     * @brief 将远端地址输出为字符串
     * 
     * @return std::string 
     */
    std::string getRemoteAddressString();

    /**
     * @brief 将本地地址输出为字符串
     * 
     * @return std::string 
     */
    std::string getLocalAddressString();

private:
    // Socket类
    Socket::ptr m_socket;
    // 是否主控 -- 是否拥有当前sock的管理权 -- 决定了当前对象析构时，是否将sock关闭
    bool m_owner;
};
} // namespace webs

#endif