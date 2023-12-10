#include "BinanceMarketClientV2.h"
#include <../sig/perf.h>

void md::BinanceUnit::sub_websocket(){
    try{
        wsClient.close();
        uri_builder builder(m_wsBaseUrl);
        wsClient.connect(builder.to_string())
        .then([&]() {
            //绑定消息回调函数
            std::function<void (const websocket_incoming_message &msg)> f;
            f = std::bind(&MarketDataBaseStruct::on_websocket_msg, this, placeholders::_1);
            wsClient.set_message_handler(f);
            //绑定ws断开函数
            std::function<void (websocket_close_status close_status,
                                const utility::string_t& reason, const std::error_code& error)> c;
            c =  std::bind(&MarketDataBaseStruct::on_close_msg,this
                    ,placeholders::_1,placeholders::_2,placeholders::_3);
            wsClient.set_close_handler(c);
        }).wait();

        //发送订阅数据,订阅过多会报错
        auto count = 0;
        auto size = subValueVec.size();
        json::value jvalue;
        jvalue["method"] = json::value::string("SUBSCRIBE");
        for(auto i=size; i > 0; -- i){
            jvalue["params"][count++] = json::value::string(subValueVec[i-1]);
            if (i == 1 || count >= 100){
                websocket_outgoing_message outMsg;
                jvalue["id"] = json::value::number(count);
                outMsg.set_utf8_message(jvalue.serialize().c_str());
                wsClient.send(outMsg).wait();
                LOG_INFO("%s send %s to %s",exchIdStr.c_str(),jvalue.serialize().c_str(), m_wsBaseUrl.c_str());        
                count = 0;
                jvalue.erase("params");
                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        m_IsConnected = true;
    }
    catch(std::exception &e){
        m_IsConnected = false;
        LOG_ERROR("%s,%s,%s,%s,%d,%s",exchIdStr.c_str(),instTypeStr.c_str(),
                  marketTypeStr.c_str(), __FUNCTION__ , __LINE__, e.what());
//        m_IsConnected = false;
//        fprintf(stderr, "%s,%s,%s\n",exchIdStr.c_str(), __FUNCTION__ , e.what());
//        LOG_ERROR("%s %s,%s",exchIdStr.c_str(), __FUNCTION__ , e.what());
    }
}

void md::BinanceUnit::ping(){
    //binance主动发送ping帧会造成失联
//    if(m_IsConnected){
//        websocket_outgoing_message outMsg;
//        outMsg.set_ping_message();
//        wsClient.send(outMsg).wait();
//    }
}

void md::BinanceUnit::pong(){
    if(m_IsConnected){
        websocket_outgoing_message outMsg;
        outMsg.set_pong_message();
        wsClient.send(outMsg).wait();
    }
}

//https://microsoft.github.io/cpprestsdk/classweb_1_1websockets_1_1client_1_1websocket__incoming__message.html
void md::BinanceUnit::on_websocket_msg(const websocket_incoming_message &in_msg){
    if (in_msg.message_type() == websocket_message_type::text_message){
        const string &s = in_msg.extract_string().get();
        HANDLE_TEXT_MSG(s)
    }
    else if(in_msg.message_type() == websocket_message_type::ping){
        LOG_DEBUG("%s ping got will reply pong",exchIdStr.c_str());
        pong();
    }
    else if(in_msg.message_type() == websocket_message_type::close){
        LOG_DEBUG("%s close got ",exchIdStr.c_str());
        m_IsConnected = false;
    }
}

void md::BinanceUnit::construct(){
    LOG_INFO("%s", getString().c_str());
    exchIdStr = ExchangeTypeEnum2StrMap[exchangeTypeEnum];
    instTypeStr = InstTypeEnum2StrMap[instTypeEnum];
    marketTypeStr = MarketTypeEnum2StrMap[marketTypeEnum];
    //先构造 ws地址
    if(subMarketTypeEnum == SMT_SPOT_MD){
        m_wsBaseUrl = BINANCE_WEBSOCKET_HOST_PUBLIC_SPOT;
        //topictag = "BINANCE-SPOTMD";
    }
    else if(subMarketTypeEnum == SMT_USDT_SWAP
            || subMarketTypeEnum == SMT_USDT_FUTURES){
        m_wsBaseUrl = BINANCE_WEBSOCKET_HOST_PUBLIC_USDT_SWAP_FUTURES;
        //topictag = "BINANCE-USDTMD";
    }
    else if(subMarketTypeEnum == SMT_USD_SWAP
            || subMarketTypeEnum == SMT_USD_FUTURES){
        m_wsBaseUrl = BINANCE_WEBSOCKET_HOST_PUBLIC_USD_SWAP_FUTURES;
        //topictag = "BINANCE-USDMD";
    }
    else{
        LOG_ERROR("not support your sub market type!");
        // cryptothrow("binance not support your sub market type!",-1);
    }
    //开始构造订阅格式,subStrVec存的是原始格式
    //subValue["method"] = json::value::string("SUBSCRIBE");
    subValueVec.clear();
    for(auto instId : subStrVec){
        string lowerOriginInstId = crypto::to_lower(instId);
        string lowerMarketType = crypto::to_lower(marketTypeStr);
        if(subMarketTypeEnum == SMT_SPOT_MD){
            if(crypto::has_str(marketTypeStr, "DEPTH") == true){
                if(crypto::str_cmp(marketTypeStr.c_str(), "DEPTH1") == true){
                    char param[32];
                    sprintf(param, "%s@bookTicker", lowerOriginInstId.c_str());
                    //subValue["params"][subCount++] = json::value::string(param);
                    subValueVec.push_back(param);
                }
                else{
                    char param[32];
                    sprintf(param, BINANCE_SPOT_MD_DEPTH_FORMAT, lowerOriginInstId.c_str() ,
                            lowerMarketType.c_str());
                    //subValue["params"][subCount++] = json::value::string(param);
                    subValueVec.push_back(param);
                }

            }
            else if(crypto::has_str(marketTypeStr, "TRADE") == true){
                char param[32];
                sprintf(param, BINANCE_SPOT_MD_TRADES_FORMAT, lowerOriginInstId.c_str() );
                //subValue["params"][subCount++] = json::value::string(param);
                subValueVec.push_back(param);
            }
            else{
                string msg = "not support " + marketTypeStr;
                LOG_ERROR("%s", msg.c_str());
            }
        }//这里usdt和busd本位都在这里处理
        else if( subMarketTypeEnum == SMT_USDT_SWAP || subMarketTypeEnum == SMT_USDT_FUTURES){
            if(crypto::has_str(marketTypeStr, "DEPTH") == true){
                if(crypto::str_cmp(marketTypeStr.c_str(), "DEPTH1") == true){
                    char param[32];
                    sprintf(param, "%s@bookTicker", lowerOriginInstId.c_str());
                    subValueVec.push_back(param);

                    //Sub FundingRate
                    sprintf(param, BINANCE_USDT_SWAP_FUTURES_MD_FUNDING_FORMAT, lowerOriginInstId.c_str());
                    subValueVec.push_back(param);
                    
                    //Sub Depth20
                    sprintf(param,BINANCE_USDT_SWAP_FUTURES_MD_DEPTH_FORMAT, lowerOriginInstId.c_str(),
                            "depth20");
                    subValueVec.push_back(param);
                }
                else{
                    char param[32];
                    sprintf(param,BINANCE_USDT_SWAP_FUTURES_MD_DEPTH_FORMAT, lowerOriginInstId.c_str(),
                            lowerMarketType.c_str());
                    //subValue["params"][subCount++] = json::value::string(param);
                    subValueVec.push_back(param);
                }
            }
            else if(crypto::has_str(marketTypeStr, "TRADE") == true){
                char param[32];
                sprintf(param,BINANCE_USDT_SWAP_FUTURES_MD_TRADES_FORMAT,lowerOriginInstId.c_str());
                //subValue["params"][subCount++] = json::value::string(param);
                subValueVec.push_back(param);
            }
            else if(crypto::has_str(marketTypeStr, "KLINE") == true){
                char param[32];
                sprintf(param,BINANCE_USDT_SWAP_FUTURES_MD_KLINE_FORMAT,lowerOriginInstId.c_str() ,
                        lowerMarketType.c_str());
                //subValue["params"][subCount++] = json::value::string(param);
                subValueVec.push_back(param);
            }
            // else if(crypto::has_str(marketTypeStr, "FUNDING") == true){
            //     if(subMarketTypeEnum == SMT_USDT_SWAP){
            //         char funding[32];
            //         sprintf(funding, BINANCE_USDT_SWAP_FUTURES_MD_FUNDING_FORMAT, lowerOriginInstId.c_str());
            //         subValue["params"][subCount++] = json::value::string(funding);
            //     }
            // }
            else if(crypto::has_str(marketTypeStr, "MBP") == true){
                char param[32];
                sprintf(param, BINANCE_UF_MD_MBP_FORMAT, lowerOriginInstId.c_str() ,
                        "depth");
                //subValue["params"][subCount++] = json::value::string(param);
                subValueVec.push_back(param);
            }

        }//币本位
        else if( subMarketTypeEnum == SMT_USD_SWAP
                 || subMarketTypeEnum == SMT_USD_FUTURES)
        {
            if(crypto::has_str(marketTypeStr, "DEPTH") == true){
                if(crypto::str_cmp(marketTypeStr.c_str(), "DEPTH1") == true){
                    char param[32];
                    sprintf(param, "%s@bookTicker", lowerOriginInstId.c_str());
                    //subValue["params"][subCount++] = json::value::string(param);
                    subValueVec.push_back(param);

                    sprintf(param, BINANCE_USDT_SWAP_FUTURES_MD_FUNDING_FORMAT, lowerOriginInstId.c_str());
                    //subValue["params"][subCount++] = json::value::string(param);
                    subValueVec.push_back(param);
                }
                else{
                    char param[32];
                    sprintf(param,BINANCE_USD_SWAP_FUTURES_MD_DEPTH_FORMAT, lowerOriginInstId.c_str(),
                            lowerMarketType.c_str());
                    //subValue["params"][subCount++] = json::value::string(param);
                    subValueVec.push_back(param);                    
                }
            }
            else if(crypto::has_str(marketTypeStr, "TRADE") == true){
                char param[32];
                sprintf(param,BINANCE_USD_SWAP_FUTURES_MD_TRADES_FORMAT,lowerOriginInstId.c_str());
                //subValue["params"][subCount++] = json::value::string(param);
                subValueVec.push_back(param);
            }
            else if(crypto::has_str(marketTypeStr, "KLINE") == true){
                char param[32];
                sprintf(param,BINANCE_USD_SWAP_FUTURES_MD_KLINE_FORMAT,lowerOriginInstId.c_str() ,
                        lowerMarketType.c_str());
                //subValue["params"][subCount++] = json::value::string(param);
                subValueVec.push_back(param);
            }
            // else if(crypto::has_str(marketTypeStr, "FUNDING") == true){
            //     if(subMarketTypeEnum == SMT_USD_SWAP){
            //         char funding[32];
            //         sprintf(funding, BINANCE_USDT_SWAP_FUTURES_MD_FUNDING_FORMAT, lowerOriginInstId.c_str());
            //         subValue["params"][subCount++] = json::value::string(funding);
            //     }
            // }
            else if(crypto::has_str(marketTypeStr, "MBP") == true){
                char param[32];
                sprintf(param, BINANCE_CF_MD_MBP_FORMAT, lowerOriginInstId.c_str() ,
                        "depth");
                //subValue["params"][subCount++] = json::value::string(param);
                subValueVec.push_back(param);
            }
        }
        else{
            string msg = "not support subMarketType: " + SubMarketTypeEnum2StrMap[subMarketTypeEnum];
            LOG_ERROR("%s", msg.c_str());
        }
    }
}

//处理消息 解析json并发送给redis或共享内存
void md::BinanceUnit::save_md_string(const DbsData& mdMsg){
    //cout << mdMsg.data << "\n";
    rapidjson::Document d;
    rapidjson::Value &rawData = d.Parse<rapidjson::kParseNumbersAsStringsFlag>(mdMsg.data);
    if(d.HasParseError() || !rawData.HasMember("data")){
        return;
    }

    string originInstId;
    const auto& data = rawData["data"];
    if(data.HasMember("s")){
        originInstId = data["s"].GetString();
    }
    else{
        string stream(rawData["stream"].GetString());
        vector<string> s_vec = crypto::split(stream,'@');
        originInstId = crypto::to_upper(s_vec[0]).c_str();
    }
    
    static auto writer = DbwBase::instance().getDBW_W();
    static thread_local auto colmap = writer->GetColumns();
    auto exchangeTypeEnum = mdMsg.h.exchangeTypeEnum;
    auto marketTypeEnum = data.HasMember("b")?DEPTH1:FUNDING_RATE;
    if (marketTypeEnum == DEPTH1 && data["e"].GetString()[0] == 'd')
        marketTypeEnum = DEPTH20;
    auto instTypeEnum = InstType_USDT_SWAP;
    
    auto symbol = dbw::MakeDBPSymbolKey(ExchangeTypeEnum2StrMap[exchangeTypeEnum]
        ,InstTypeEnum2StrMap[instTypeEnum],originInstId.c_str());
    auto iter = colmap.find(symbol);
    if (iter == colmap.end())
        return;
    auto dbt = iter->second;
    auto& cmd = dbt->d;
    //auto& info = dbt->i;
    auto wdata = writer->Fetch(symbol);

    if(marketTypeEnum == FUNDING_RATE){
        auto& fundingRate = wdata->d;
        memcpy(&fundingRate,&cmd,sizeof(cmd));
        fundingRate.body.fundingRate.fundingRate = stod(data["r"].GetString());
        fundingRate.body.fundingRate.nextFundingRate = 0;//stod(data["B"].GetString());
        fundingRate.body.fundingRate.fundingTime = stol(data["T"].GetString()) * 1000;
        fundingRate.body.fundingRate.ts = stol(data["E"].GetString()) * 1000;
        fundingRate.header.marketTypeEnum = marketTypeEnum;
        fundingRate.body.fundingRate.marketTypeEnum = marketTypeEnum;
        fundingRate.body.fundingRate.tsNet = mdMsg.__updatestamp/1000;
        fundingRate.body.fundingRate.tsParse = crypto::getCurrentTime();  
    }
    else{
        if(marketTypeEnum == DEPTH1){
            cmd.body.depth1.bp1 = stod(data["b"].GetString());
            cmd.body.depth1.bv1 = stod(data["B"].GetString());
            cmd.body.depth1.ap1 = stod(data["a"].GetString());
            cmd.body.depth1.av1 = stod(data["A"].GetString());
            if (mdMsg.h.instTypeEnum == SWAP || mdMsg.h.instTypeEnum == FUTURES){
                cmd.body.depth1.ts = stol(data["T"].GetString()) * 1000;
            }
            else
                cmd.body.depth1.ts = 0;
        }
        else if (marketTypeEnum == DEPTH20){
            DEPTHSET(5,2)
            DEPTHSET(5,3)
            DEPTHSET(5,4)
            DEPTHSET(5,5)
            
            if (mdMsg.h.instTypeEnum == SWAP || mdMsg.h.instTypeEnum == FUTURES){
                cmd.body.depth5.ts = stol(data["T"].GetString()) * 1000;
            }
            else
                cmd.body.depth5.ts = 0;
            if (cmd.body.depth5.ts > cmd.body.depth1.ts){
                cmd.body.depth5.bp1 = stod(data["b"][0][0].GetString());
                cmd.body.depth5.bv1 = stod(data["b"][0][1].GetString());
                cmd.body.depth5.ap1 = stod(data["a"][0][0].GetString());
                cmd.body.depth5.av1 = stod(data["a"][0][1].GetString());
            }
        }
        cmd.header.marketTypeEnum = marketTypeEnum;
        cmd.body.depth1.marketTypeEnum = marketTypeEnum;
        cmd.body.depth1.tsNet = mdMsg.__updatestamp/1000;
        cmd.body.depth1.tsParse = crypto::getCurrentTime();
        memcpy(&(wdata->d),&cmd,sizeof(cmd));
    }

    writer->Commit(wdata->__mnodeid);
    //cout << " ,6:" << crypto::getCurrentTime() - mdMsg.__updatestamp/1000 << endl;
    static thread_local pf::performance p(topictag);
    p.Update(crypto::getCurrentTime()-mdMsg.__updatestamp/1000);

    if (originInstId == "BTCUSDT")
        cout << "symbol:" << symbol << " marketTypeEnum:"  << marketTypeEnum \
            << " b1:[" << cmd.body.depth5.bp1 << "," << cmd.body.depth5.bv1  \
            << "] a1:[" << cmd.body.depth5.ap1 << "," << cmd.body.depth5.av1 \
            << "] b2:[" << cmd.body.depth5.bp2 << "," << cmd.body.depth5.bv2 \
            << "] a2:[" << cmd.body.depth5.ap2 << "," << cmd.body.depth5.av2 \
            << "] ts1:" << cmd.body.depth1.ts << " ts5:" << cmd.body.depth5.ts << endl;
}

void md::BinanceMarketClientV2::construct() {
    for(auto marketType : _marketTypeVec){
        for(auto instType : _instTypeVec){
            //存储有效的交易对
            vector<string> validInstIdVec;
            for(auto instId : _instIdVec){
                //先过滤一遍，筛选掉smc中不存在的交易对
                InstrumentInfo info;
                if(smc->get_instrument_info(exchId, instType.c_str(),
                                            instId.c_str(), info) == true){
                    string lowerOriginInstId = crypto::to_lower(info.originInstId);
                    validInstIdVec.push_back(info.originInstId);
//                    cout << exchId  << "." << instType  << "." << marketType << "." <<lowerOriginInstId << endl;
                }
                else{
                    LOG_ERROR("not found %s.%s.%s smc info", exchId, instType.c_str(),
                              instId.c_str());
                }
            }
            //再根据instType确定订阅类型
            //如果是spot，不需要区分，永续和交割需要进一步区分
            if(crypto::str_cmp(instType.c_str(), "SPOT") == true &&
                    crypto::has_str(marketType.c_str(), "FUNDING") == false){
                //现货没有费率信息
                //有效的交易对数量
                size_t validSize = validInstIdVec.size();
                //需要的token unit单元，若不能取整则单元需要多加一个
                size_t unitSize = validSize % tokenLot == 0 ? validSize / tokenLot : validSize / tokenLot + 1;
                for(size_t us = 0; us < unitSize; us++){
                    BinanceUnit unit = {ExchangeTypeStr2EnumMap[exchId],InstTypeStr2EnumMap[instType],
                                    MarketTypeStr2EnumMap[marketType]};
                    size_t startValidNum = tokenLot * (us);
                    size_t endValidNum = tokenLot * (us + 1) > validSize ? validSize : tokenLot * (us + 1);
                    for(size_t i = startValidNum;i < endValidNum; i++){
                        unit.subStrVec.push_back(validInstIdVec[i]);
                    }
                    unit.subMarketTypeEnum = SMT_SPOT_MD;
                    unit.topictag = "BINANCE-SPOTMD-"+to_string(us);
                    tokenUnitVec.push_back(unit);
                }
            }
            else if( (crypto::str_cmp(instType.c_str(), "SWAP")  )
            || (crypto::str_cmp(instType.c_str(), "FUTURES")
                && crypto::has_str(marketType.c_str(), "FUNDING") == false ) ){//交割合约没有费率信息
                //这里需要区分币本位和其他本位（目前主要是USDT和BUSD本位）
                vector<string> coinStrVec, notCoinStrVec;
                for(string inst : validInstIdVec){
                    InstrumentInfo info;
                    if(smc->get_instrument_info(exchId, instType.c_str(),
                                                inst.c_str(), info) == true){
                        if(crypto::str_cmp(info.quote, "USD")){
                            //币本位
                            coinStrVec.push_back(inst);
                        }
                        else{
                            //usdt或者busd本位
                            notCoinStrVec.push_back(inst);
                        }
                    }
                    else{
                        //这里不会执行到,因为上面已经筛选过了
                        string msg("not found ");
                        msg.append(exchId).append(".") .append(instType).append(".")
                        .append(inst).append(" smc info");
                        LOG_ERROR("%s", msg.c_str());
                    }
                }
                if(coinStrVec.size() > 0){
                    //有效的交易对数量
                    size_t validSize = coinStrVec.size();
                    //需要的token unit单元数量，若不能取整则单元需要多加一个
                    size_t unitSize = validSize % tokenLot == 0 ? validSize / tokenLot : validSize / tokenLot + 1;
                    for(size_t us = 0; us < unitSize; us++){
                        BinanceUnit unit = {ExchangeTypeStr2EnumMap[exchId],InstTypeStr2EnumMap[instType],
                                            MarketTypeStr2EnumMap[marketType]};
                        size_t startValidNum = tokenLot * (us);
                        size_t endValidNum = tokenLot * (us + 1) > validSize ? validSize : tokenLot * (us + 1);
                        for(size_t i = startValidNum;i < endValidNum; i++){
                            //将需要订阅的交易对，塞到struct
                            unit.subStrVec.push_back(coinStrVec[i]);
                        }
                        if(crypto::str_cmp(instType.c_str(), "SWAP")){
                            unit.subMarketTypeEnum = SMT_USD_SWAP;
                        }
                        else{
                            unit.subMarketTypeEnum = SMT_USD_FUTURES;
                        }
                        unit.topictag = "BINANCE-USDMD-"+to_string(us);
                        tokenUnitVec.push_back(unit);
                    }
                }
                if(notCoinStrVec.size() > 0){
                    //有效的交易对数量
                    size_t validSize = notCoinStrVec.size();
                    //需要的token unit单元数量，若不能取整则单元需要多加一个
                    size_t unitSize = validSize % tokenLot == 0 ? validSize / tokenLot : validSize / tokenLot + 1;
                    for(size_t us = 0; us < unitSize; us++){
                        BinanceUnit unit = {ExchangeTypeStr2EnumMap[exchId],InstTypeStr2EnumMap[instType],
                                            MarketTypeStr2EnumMap[marketType]};
                        size_t startValidNum = tokenLot * (us);
                        size_t endValidNum = tokenLot * (us + 1) > validSize ? validSize : tokenLot * (us + 1);
                        for(size_t i = startValidNum;i < endValidNum; i++){
                            //将需要订阅的交易对，塞到struct
                            unit.subStrVec.push_back(notCoinStrVec[i]);
                        }
                        //
                        if(crypto::str_cmp(instType.c_str(), "SWAP")){
                            unit.subMarketTypeEnum = SMT_USDT_SWAP;
                        }
                        else{
                            unit.subMarketTypeEnum = SMT_USDT_FUTURES;
                        }
                        unit.topictag = "BINANCE-USDTMD-"+to_string(us);
                        tokenUnitVec.push_back(unit);
                    }
                }
            }
        }
    }
}

void md::BinanceMarketClientV2::start(){
    construct();
    for(auto &unit: tokenUnitVec){
        //共用一个smc即可
        unit.smc = smc;
        unit.construct();
        if(unit.subValueVec.size()>0){
        //if(unit.subValue.has_field("params")){
            unit.start();
            usleep(100);
        }
        else{
            LOG_INFO("no need to start: %s",unit.getString().c_str());
        }
    }
}

void md::BinanceMarketClientV2::stop(){
    for(auto &unit: tokenUnitVec){
        unit.stop();
    }
}