//==============================================================================
// 
//==============================================================================
#ifndef BFAPI_ORDERS_HPP
#define BFAPI_ORDERS_HPP
#include <string>
#include <cstdint>

namespace bfapi {
namespace orders {
    
struct order_instruction {
	// Base class for order instruction
	
	std::int64_t selection_id;
	bool lay_side;	

	virtual std::string as_json_string() const = 0;

	order_instruction(const std::int64_t& id, const bool lay) : selection_id(id), lay_side(lay) {}

};
    
struct limit_order_instruction : public order_instruction {
	
	double size;
	double price;
	bool persist;
	std::string co_ref_string;

	virtual std::string as_json_string() const
	{
		// Ugly string dump but will suffice for the time being to test things
		std::string data = "{\"selectionId\":" + std::to_string(selection_id) + ",\"side\":\"" + (lay_side ? "LAY" : "BACK") + "\",\"orderType\":\"LIMIT\"";
		data += (",\"limitOrder\":{\"size\":" + std::to_string(size) + ",\"price\":" + std::to_string(price));
		data += (",\"persistenceType\":\"" + std::string(persist ? "PERSIST" : "LAPSE") + "\"}");
		data += (co_ref_string.empty() ? "" : (",\"customerOrderRef\":\"" + co_ref_string + "\"") + "}");
		return data;
	}

	limit_order_instruction(const std::int64_t& id, 
							const bool lay,
							const double& stake,
							const double& odds,
							const bool persist_order,
							const std::string& coref) : order_instruction(id,lay), size(stake), price(odds), persist(persist_order), co_ref_string(coref) {}

	// Example of a order instruction
	// "{\"selectionId\":50198,\"side\":\"LAY\",\"orderType\":\"LIMIT\",\"limitOrder\":{\"size\":2.0,\"price\":1.01,\"persistenceType\":\"LAPSE\"}}";
};

struct place_limit_orders_request {
	std::string market_id;
	bool async_placement;
	std::vector<limit_order_instruction> instructions_list;	
	
	std::string as_json_string() const
	{
		std::string request_body = "{\"marketId\":\"" + market_id + "\",\"instructions\":[";
        for (const orders::limit_order_instruction& instruction : instructions_list)
        {	
			request_body += instruction.as_json_string() + ",";
		}		
		request_body.pop_back();		
		if (async_placement)
		{
			request_body += "],\"async\":true}";
		}
		else
		{
			request_body += "]}";
		}		
		return request_body;		
	}
	
	place_limit_orders_request(const std::string& mid, bool async, const std::vector<limit_order_instruction>& bets) : market_id(mid),async_placement(async),instructions_list(bets) {}
};
    
} // end of namespace bfapi::orders
} // end of namespace bfapi

#endif
