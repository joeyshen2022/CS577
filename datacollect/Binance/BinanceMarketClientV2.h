#pragma once
#include "base/market_base_v2.h"


//usdt swap futures
#define BINANCE_WEBSOCKET_HOST_PUBLIC_USDT_SWAP_FUTURES "wss://fstream.binance.com/stream?"
//spot
#define BINANCE_WEBSOCKET_HOST_PUBLIC_SPOT "wss://stream.binance.com:9443/stream?"

//现货
#define BINANCE_REST_HOST_PUBLIC_SPOT "https://api.binance.com/api/v3/"
//u本位
#define BINANCE_REST_HOST_PUBLIC_UF "https://fapi.binance.com/fapi/v1/"
//币本位
#define BINANCE_REST_HOST_PUBLIC_CF "https://dapi.binance.com/dapi/v1/"

//usd swap futures
#define BINANCE_WEBSOCKET_HOST_PUBLIC_USD_SWAP_FUTURES "wss://dstream.binance.com/stream?"


//1inchusdt@depth20@100ms
#define BINANCE_SPOT_MD_DEPTH_FORMAT "%s@%s@100ms"
//1inchusdt@trade
#define BINANCE_SPOT_MD_TRADES_FORMAT "%s@trade"
//1inchusdt@kline_1m
#define BINANCE_SPOT_MD_KLINE_FORMAT "%s@%s"
//1inchusdt@depth20@1000ms
#define BINANCE_SPOT_MD_MBP_FORMAT "%s@%s@1000ms"
//u本位和币本位格式一样
#define BINANCE_UF_MD_MBP_FORMAT "%s@%s@500ms"

#define BINANCE_CF_MD_MBP_FORMAT "%s@%s@500ms"
//1inchusdt@depth20@100ms
#define BINANCE_USDT_SWAP_FUTURES_MD_DEPTH_FORMAT "%s@%s@100ms"
//1inchusdt@aggTrade
#define BINANCE_USDT_SWAP_FUTURES_MD_TRADES_FORMAT "%s@aggTrade"
//1inchusdt@kline_1m
#define BINANCE_USDT_SWAP_FUTURES_MD_KLINE_FORMAT "%s@%s"

#define BINANCE_USDT_SWAP_FUTURES_MD_FUNDING_FORMAT "%s@markPrice@1s"

//1inchusd@depth20@100ms
#define BINANCE_USD_SWAP_FUTURES_MD_DEPTH_FORMAT "%s@%s@100ms"
//1inchusd@aggTrade
#define BINANCE_USD_SWAP_FUTURES_MD_TRADES_FORMAT "%s@aggTrade"
//1inchusd@kline_1m
#define BINANCE_USD_SWAP_FUTURES_MD_KLINE_FORMAT "%s@%s"

#define BINANCE_USD_SWAP_FUTURES_MD_FUNDING_FORMAT "%s@markPrice@1s"


namespace md {
    struct BinanceUnit : public MarketDataBaseStruct{
        BinanceUnit(ExchangeType exchangeType, InstType instType, MarketType marketType){
            this->exchangeTypeEnum = exchangeType;
            this->instTypeEnum = instType;
            this->marketTypeEnum = marketType;
        }
        //计数用的
        int subCount = 0;
        int subId = crypto::get_int_rand(100,10000);
        //发给交易所的订阅数据
        vector<string> subValueVec;
        //json::value subValue;
        virtual void construct();
        virtual void sub_websocket();
        virtual void ping();
        virtual void pong();
        virtual void on_websocket_msg(const websocket_incoming_message &in_msg);
        virtual void save_md_string(const DbsData& mdMsg);
    };

    class BinanceMarketClientV2 : public MarketDataBaseClass {
    public:
        BinanceMarketClientV2(exchange_node& exchange,sm::SecurityManager* _smc)
                : MarketDataBaseClass(exchange.instType, exchange.marketType, exchange.instId,exchange.tokenLot,_smc){
            strcpy(exchId, "BINANCE");
        }
        virtual ~BinanceMarketClientV2(){};

    public:
        virtual void start();
        virtual void stop();
    private:
        //请求全量rest地址
        //spot现货
        string m_spotRestBaseUrl;
        //u本位合约
        string m_ufRestBaseUrl;
        //币本位合约
        string m_cfRestBaseUrl;
        //存储需要请求全量的instId，原始格式，大写
        vector<string> reqSpotInstIdVec ;
        //u本位swap
        vector<string> reqUSwapInstIdVec;
        //u本位futures
        vector<string> reqUFuturesInstIdVec;
        //c本位swap
        vector<string> reqCSwapInstIdVec;
        //c本位futures
        vector<string> reqCFuturesInstIdVec;
        // //mbp全量数据只能用一个线程串行轮询全量rest接口，因为如果多线程的话会触发限频
        // virtual void req_mbp();
        // virtual void req_mbp_by_subMarketType(vector<string> &reqInstIdVec, md::SubMarketType subMarketType);

        //确定好每个unit单元的有效originInstId
        virtual void construct();
        //打印监控一些队列的堆积情况
        //virtual void print_stat();

        vector<BinanceUnit> tokenUnitVec;
    };
}