/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "eosBlack.token.hpp"

namespace eosblack {

void blacktoken::create( account_name issuer,
                    asset        maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "blacktoken with symbol already exists" );

	blacktoken_initialize();

    statstable.emplace( _self, [&]( auto& s ) {
		    s.supply.symbol = maximum_supply.symbol;
		    s.max_supply    = maximum_supply;
		    s.issuer        = issuer;
		    });
}


void blacktoken::issue( account_name to, asset quantity, string memo )
{
	auto sym = quantity.symbol;
	eosio_assert( is_account( to ), "to account does not exist");
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "blacktoken with symbol does not exist, create blacktoken before issue" );
    const auto& st = *existing;

	require_auth( st.issuer );

	eosio_assert( is_runnable() == true, "contract was frozen");

	eosio_assert( is_blacklist(to) == false, "not allowed issue to black list");


	eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

	// check locking amount
	eosio_assert( quantity.amount <= available_balance(st.max_supply.amount - st.supply.amount),
					"currency has locking... please check lock time and amount");

	statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
       SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo});
	}
}

void blacktoken::transfer( account_name from,
	                       account_name to,
	                       asset        quantity,
	                       string       memo )
{

	eosio_assert( from != to, "cannot transfer to self" );

	require_auth( from );

	eosio_assert( is_account( to ), "to account does not exist");

    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

	require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	eosio_assert( is_runnable() == true, "contract was frozen");

	// check black list.
	eosio_assert( is_permit_transfer( from, to ) == true ,"not allow transfer!");

    sub_balance( from, quantity );
	add_balance( to, quantity, from );
}

void blacktoken::supplement( asset quantity )
{
	auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );

	auto sym_name = sym.name();

	stats statstable( _self, sym_name );

	auto existing = statstable.find( sym_name );

	eosio_assert( existing != statstable.end(), "blacktoken with symbol does not exist, create blacktoken before supplement" );

	eosio_assert( is_runnable() == true, "contract was frozen");

	const auto& st = *existing;

    require_auth( st.issuer);
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, 0, [&]( auto& s ) {
       s.max_supply	    += quantity;
    });
}

void blacktoken::burn( asset quantity )
{
	auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );

	eosio_assert( is_runnable() == true, "contract was frozen");

    auto sym_name = sym.name();

	stats statstable( _self, sym_name );

	auto existing = statstable.find( sym_name );

	eosio_assert( existing != statstable.end(), "blacktoken with symbol does not exist, create blacktoken before burn" );

	const auto& st = *existing;

    require_auth( st.issuer);
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

	eosio_assert( (st.max_supply.amount - st.supply.amount) >= quantity.amount , "Burn has allow when value of burn have big amount more than remain value");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.max_supply	    -= quantity;
    });
}

void blacktoken::sub_balance( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void blacktoken::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

bool blacktoken::is_permit_transfer( account_name to, account_name from )
{
	bool is_permit = false;

	if ( is_blacklist(to) == false && is_blacklist(from) == false)
	{
		is_permit = true;
	}

	return is_permit;
}

void blacktoken::addblklst( account_name user ,asset asset_for_symbol )
{
	eosio_assert(is_blacklist(user) == false, "account_name already exist in blacklist");

	auth_check(asset_for_symbol);

	auto tc = get_token_config();

	tc.blacklist.push_back(user);

	update_token_config(tc);
}

void blacktoken::rmvblklst( account_name user ,asset asset_for_symbol )
{
	auth_check(asset_for_symbol);

	auto tc = get_token_config();

	vector<account_name>::iterator iter;

	iter = find(tc.blacklist.begin(), tc.blacklist.end(), user);

	eosio_assert( iter != tc.blacklist.end(), "account_name not exist in blacklist");

	tc.blacklist.erase(iter);

	update_token_config(tc);
}

inline void blacktoken::stop( asset asset_for_symbol )
{
	auth_check(asset_for_symbol);

	auto token_config = get_token_config();

	eosio_assert( token_config.is_runnable == true, "contract was frozen");
	eosio::print( " contract going to freeze! \n " );
	token_config.is_runnable = false;

	update_token_config(token_config);
}


inline void blacktoken::restart( asset asset_for_symbol )
{
	auth_check(asset_for_symbol);

	auto token_config = get_token_config();

	eosio_assert( token_config.is_runnable == false, "contract is runing");
	eosio::print( " contract wake up! \n " );
	token_config.is_runnable = true;

	update_token_config(token_config);
}

bool blacktoken::is_blacklist( account_name user)
{
	auto tc = get_token_config();

	vector<account_name>::iterator iter;
	iter = find(tc.blacklist.begin(), tc.blacklist.end(), user);

	return iter == tc.blacklist.end() ? false : true ;

}

blacktoken::tokencfg blacktoken::get_token_config()
{
	tokencfg tc;

	if(tokenconfig.exists()) 
	{
		tc = tokenconfig.get();
	}
	else
	{
		tc = tokencfg{};
		if ( tokenconfig.exists()) {
			auto old_tc = tokenconfig.get();
			tc.is_runnable = old_tc.is_runnable;
            tc.create_time = old_tc.create_time;
			tokenconfig.remove();
		}
		tokenconfig.set(tc, _self);
	}

	return tc;
}

void blacktoken::update_token_config(blacktoken::tokencfg &new_token_config)
{
	tokenconfig.set(new_token_config, _self);
}

void blacktoken::blacktoken_initialize()
{
	auto token_config = get_token_config();
	token_config.is_runnable	= true;
	token_config.create_time	= time_point_sec(now()).utc_seconds;

	token_config.max_period		= 12;
    token_config.period_uint	= 30 * 86400; // 30 day
    token_config.lock_period	= 3; // 90 days holding after unlock 1 bilions per 30 days
    token_config.balance_uint	= 1000000000000;
    token_config.lock_balance	= 9000000000000; // 9 bilions - it unlock when token created after 360 days

	update_token_config(token_config);
	eosio::print("token config initialize : ", " token create time  ", token_config.create_time, " (sec)\n");
	eosio::print("token config initialize : ", " max_period	  ", token_config.max_period	," \n");	
	eosio::print("token config initialize : ", " lock_period  ", token_config.lock_period	," \n");	
	eosio::print("token config initialize : ", " period_uint  ", token_config.period_uint, " (sec)\n");	
	eosio::print("token config initialize : ", " amount_uint  ", token_config.balance_uint, " \n");	
	eosio::print("token config initialize : ", " lock_balance ", token_config.lock_balance, " \n");	

}

bool blacktoken::is_runnable()
{
	auto token_config = get_token_config();
	return token_config.is_runnable ==  true ? true : false;
}
/*
 *                                Token holding balance per period.
 * 
 *===============================================================================================================
 * day after    |  30d |  60d |  90d |  120d |  150d |  180d |  210d |  240d | 270d  |  300d |  330d | 360d | 
 * --------------------------------------------------------------------------------------------------------------
 * period       |  1p  |  2p  |  3p  |   4p  |   5p  |   6p  |   7p  |   8p  |   9p  |  10p  |  11p  |  12p | 
 * --------------------------------------------------------------------------------------------------------------
 * hold balance | 1.8b | 1.8b | 1.8b |  1.7b |  1.6b |  1.5b |  1.4b |  1.3b |  1.2b |  1.1b |  1.0b | 0.9b | 0b
 *===============================================================================================================
 *
 *
 */
uint64_t blacktoken::available_balance( uint64_t contract_balance )
{
	uint64_t available_balance;

	auto tc = get_token_config();

	if ( tc.max_period > 0)
	{
		uint32_t current_time = time_point_sec(now()).utc_seconds;

		if ( ((current_time - tc.create_time) / (tc.lock_period * tc.period_uint)) > 0)
		{
			uint32_t current_period = (current_time - tc.create_time) / tc.period_uint + 1;

			if ( current_period <= tc.max_period)
			{
				uint64_t current_lock_balance =  tc.balance_uint * (tc.max_period - current_period);

				available_balance = contract_balance - current_lock_balance - tc.lock_balance ;

				eosio::print(" period   : ", current_period, " / ",  tc.max_period, " available balance : ", available_balance, " ");
			}
			else
			{
				available_balance = contract_balance;

				if (tc.max_period > 0)
				{
					tc.max_period = 0;
					update_token_config(tc);
				}
			}
		}
		else
		{
			// hold 18 billions within 90 days.
			available_balance = contract_balance - ( (tc.max_period - tc.lock_period) * tc.balance_uint) - tc.lock_balance;
		}
	}

	return available_balance;

}

void blacktoken::auth_check( asset asset_for_symbol)
{
	auto sym = asset_for_symbol.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    auto sym_name = sym.name();
	stats statstable( _self, sym_name );

    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "does not exist blacktoken with symbol" );

	const auto& st = *existing;
	require_auth( st.issuer );
}

} // namespace eosblack

EOSIO_ABI( eosblack::blacktoken, (create)(issue)(transfer)(addblklst)(rmvblklst)(stop)(restart)(supplement)(burn))
