/*
 *  This file is part of clearskies_core file synchronization program
 *  Copyright (C) 2014 Pedro Larroy

 *  clearskies_core is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  clearskies_core is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public License
 *  along with clearskies_core.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "messagecoder.hpp"
#include "jsoncons/json.hpp"
#include <cassert>

using namespace std;

namespace cs
{
namespace message
{

class CoderImpl
{
public:
    virtual ~CoderImpl() {};
    virtual std::unique_ptr<Message> decode_msg(bool, const char*, size_t, const char*, size_t) = 0;
    virtual std::string encode_msg(const Message&) = 0;
};

namespace coder
{
/************************** JSON Implementation **************************/

namespace jsoni
{

/*** decode json -> msg ***/

void decode(const jsoncons::json& json, Unknown& msg)
{
    // save the json message recoded for inspection
    ostringstream json_os;
    jsoncons::json_serializer serializer(json_os, true);
    json.to_stream(serializer);
    msg.m_content = json_os.str();
}

void decode(const jsoncons::json& json, InternalStart& msg)
{
}

void decode(const jsoncons::json& json, Ping& msg)
{
    msg.m_timeout = json["timeout"].as_int();
}

void decode(const jsoncons::json& json, Greeting& msg)
{
    msg.m_software = json["software"].as_string();
    msg.m_protocol = json["protocol"].as_vector<int>();
    msg.m_features = json["features"].as_vector<string>();
}

// TODO use a stringify macro to save some code
void decode(const jsoncons::json& json, Start& msg)
{

    msg.m_software = json["software"].as_string();
    msg.m_protocol = json["protocol"].as_int();
    msg.m_features = json["features"].as_vector<string>();
    msg.m_id = json["id"].as_string();
    msg.m_access = json["access"].as_string();
    msg.m_peer = json["peer"].as_string();
}

void decode(const jsoncons::json& json, CannotStart& msg)
{
}

void decode(const jsoncons::json& json, StartTLS& msg)
{
    msg.m_peer = json["peer"].as_string();
    msg.m_access = maccess_from_string(json["access"].as_string());
}

void decode(const jsoncons::json& json, Identity& msg)
{
    msg.m_name = json["name"].as_string();
    msg.m_time = json["time"].as_int();
}

void decode(const jsoncons::json& json, Keys& msg)
{
    msg.m_access = maccess_from_string(json["access"].as_string());
    msg.m_share_id = json["share_id"].as_string();

    auto ro = json["read_only"];
    msg.m_ro_psk = ro["psk"].as_string();
    msg.m_ro_rsa = ro["rsa"].as_string();

    auto rw = json["read_write"];
    msg.m_rw_public_rsa = rw["public_rsa"].as_string();
}

void decode(const jsoncons::json& json, Keys_Acknowledgment& msg)
{
}

/*** encode msg -> json ***/

void encode_type(const Message& msg, jsoncons::json& json)
{
    namespace jc = jsoncons;
    json = jc::json::an_object;
    json["type"] = mtype_to_string(msg.type());
}

void encode(const Unknown& msg, jsoncons::json& json)
{
    assert(0);
}

void encode(const InternalStart& msg, jsoncons::json& json)
{
    encode_type(msg, json);
}

void encode(const Ping& msg, jsoncons::json& json)
{
    using namespace jsoncons;
    encode_type(msg, json);
    json["timeout"] = msg.m_timeout;
}

template<class It>
jsoncons::json to_array(It begin, It end)
{
    auto result = jsoncons::json::make_array();
    for (auto i = begin; i != end; ++i)
        result.add(jsoncons::json(*i));
    return result;
}

void encode(const Greeting& msg, jsoncons::json& json)
{
    using namespace jsoncons;
    encode_type(msg, json);
    json["software"] = msg.m_software;
    //json["protocol"] = to_array(msg.m_protocol.begin(), msg.m_protocol.end());
    json["protocol"] = jsoncons::json(msg.m_protocol.begin(), msg.m_protocol.end());
    json["features"] = jsoncons::json(msg.m_features.begin(), msg.m_features.end());
}

void encode(const Start& msg, jsoncons::json& json)
{
    using namespace jsoncons;
    encode_type(msg, json);
    json["software"] = msg.m_software;
    json["protocol"] = to_string(msg.m_protocol);
    json["features"] = jsoncons::json(msg.m_features.begin(), msg.m_features.end());
    json["id"] = msg.m_id;
    json["access"] = msg.m_access;
    json["peer"] = msg.m_peer;
}

void encode(const CannotStart& msg, jsoncons::json& json)
{
    encode_type(msg, json);
}

void encode(const StartTLS& msg, jsoncons::json& json)
{
    using namespace jsoncons;
    encode_type(msg, json);
    json["peer"] = msg.m_peer;
    json["access"] = maccess_to_string(msg.m_access);
}

void encode(const Identity& msg, jsoncons::json& json)
{
    using namespace jsoncons;
    encode_type(msg, json);
    json["name"] = msg.m_name;
    json["time"] = to_string(msg.m_time);
}

void encode(const Keys& msg, jsoncons::json& json)
{
    using namespace jsoncons;
    encode_type(msg, json);
    json["access"] = maccess_to_string(msg.m_access);
    json["share_id"] = msg.m_share_id;

    jsoncons::json ro;
    ro["psk"] = msg.m_ro_psk;
    ro["rsa"] = msg.m_ro_rsa;
    json["read_only"] = ro;

    jsoncons::json rw;
    rw["public_rsa"] = msg.m_rw_public_rsa;
    json["read_write"] = rw;
}

void encode(const Keys_Acknowledgment& msg, jsoncons::json& json)
{
    assert(0);
}

// FIXME implement rest of messages


class JSONCoder: public CoderImpl, public ConstMessageVisitor
{
friend class Message;
public:
    JSONCoder():
        m_encoded_msg()
    {}


    std::unique_ptr<Message> decode_msg(bool, const char*, size_t, const char*, size_t) override;
    std::string encode_msg(const Message&) override;
    void reset()
    {
        m_encoded_msg.clear();
    }

protected:
    void visit(const Unknown&) override;
    void visit(const InternalStart&) override;
    void visit(const Ping&) override;
    void visit(const Greeting&) override;
    void visit(const Start&) override;
    void visit(const CannotStart&) override;
    void visit(const StartTLS&) override;
    void visit(const Identity&) override;
    void visit(const Keys&) override;
    void visit(const Keys_Acknowledgment&) override;
    // FIXME implement rest of messages

private:
    std::string m_encoded_msg;
};


std::unique_ptr<Message> JSONCoder::decode_msg(bool payload, const char* encoded, size_t encoded_sz, const char* signature, size_t signature_sz)
try
{
    const auto json = jsoncons::json::parse_string(string(encoded, encoded_sz));

    MType type = MType::UNKNOWN;
    if (json.has_member("type"))
        type = mtype_from_string(json["type"].as_string());

    unique_ptr<Message> msg;
    switch(type)
    {
    case MType::INTERNAL_START:
    {
        auto xmsg = make_unique<InternalStart>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::PING:
    {
        auto xmsg = make_unique<Ping>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::GREETING:
    {
        auto xmsg = make_unique<Greeting>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::START:
    {
        auto xmsg = make_unique<Start>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::CANNOT_START:
    {
        auto xmsg = make_unique<CannotStart>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::STARTTLS:
    {
        auto xmsg = make_unique<StartTLS>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::IDENTITY:
    {
        auto xmsg = make_unique<Identity>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::KEYS:
    {
        auto xmsg = make_unique<Keys>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    case MType::KEYS_ACKNOWLEDGMENT:
    {
        auto xmsg = make_unique<Keys>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }

    // FIXME implement rest of messages
    case MType::MANIFEST:
    case MType::GET_MANIFEST:
    case MType::MANIFEST_CURRENT:
    default:
    case MType::UNKNOWN:
        auto xmsg = make_unique<Unknown>();
        decode(json, *xmsg);
        msg = move(xmsg);
        break;
    }
    msg->m_payload = payload;
    msg->m_signature.assign(signature, signature_sz);
    return std::move(msg);
}
catch(const jsoncons::json_exception& e)
{
    throw CoderError(fs("JSONCoder::decode JSON parse error: " << e.what()));
}
catch(const std::runtime_error& e)
{
    throw CoderError(fs("JSONCoder::decode runtime_error: " << e.what()));
}
catch(const std::exception & e)
{
    throw CoderError(fs("JSONCoder::decode exception: " << e.what()));
}


namespace
{

std::string json_2_str(const jsoncons::json& json)
{
    ostringstream json_os;
    jsoncons::json_serializer serializer(json_os, false); // no indent
    json.to_stream(serializer);
    return json_os.str();
}

}


std::string JSONCoder::encode_msg(const Message& msg)
{
    char prefix = 0;
    if (! msg.payload() && ! msg.signature())
        prefix = 'm';
    else if (msg.payload() && ! msg.signature())
        prefix = '!';
    else if (! msg.payload() &&  msg.signature())
        prefix = 's';
    else if (msg.payload() &&  msg.signature())
        prefix = '$';

    // m3:{}\n
    msg.accept(*this);
    ostringstream result;
    result << prefix;
    assert(m_encoded_msg.size() <= Message::MAX_SIZE);
    result << m_encoded_msg.size();
    result << ':';
    result << m_encoded_msg;
    result << '\n';
    if (msg.signature())
        result << msg.m_signature << '\n';

    /******/
    // reset m_encoded_msg
    reset();
    /******/

    return result.str();
}

#define ENCXX\
    do {\
        jsoncons::json json;\
        encode(x, json);\
        m_encoded_msg = json_2_str(json);\
    } while(0);

void JSONCoder::visit(const Unknown&)
{
    assert(0);
}

void JSONCoder::visit(const InternalStart& x)
{
    ENCXX;
}

void JSONCoder::visit(const Ping& x)
{
    ENCXX;
}

void JSONCoder::visit(const Greeting& x)
{
    ENCXX;
}

void JSONCoder::visit(const Start& x)
{
    ENCXX;
}

void JSONCoder::visit(const CannotStart& x)
{
    ENCXX;
}

void JSONCoder::visit(const StartTLS& x)
{
    ENCXX;
}

void JSONCoder::visit(const Identity& x)
{
    ENCXX;
}

void JSONCoder::visit(const Keys& x)
{
    ENCXX;
}

void JSONCoder::visit(const Keys_Acknowledgment& x)
{
    ENCXX;
}


// FIXME implement rest of messages


} // end ns json
/************************** END JSON Implementation **************************/
} // end ns coder


Coder::Coder(CoderType type):
    m_p()
{
    switch(type)
    {
        case CoderType::JSON:
            m_p = make_unique<coder::jsoni::JSONCoder>();
            break;

        default:
            assert(0);
    }
}

// needs to be defined here because of m_p and CoderImpl fwd declaration
Coder::~Coder()
{
}


std::unique_ptr<Message> Coder::decode_msg(bool payload, const char* encoded, size_t encoded_sz, const char* signature, size_t signature_sz)
{
    return m_p->decode_msg(payload, encoded, encoded_sz, signature, signature_sz);
}

std::string Coder::encode_msg(const Message& m) const
{
    return m_p->encode_msg(m);
}



} // end ns
} // end ns
