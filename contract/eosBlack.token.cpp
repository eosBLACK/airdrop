#include "eosBlack.token.hpp"

namespace eosblack {

ACTION blacktoken::create( name issuer, asset maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats_t statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "blacktoken with symbol already exists" );

	blacktoken_initialize();

    statstable.emplace( _self, [&]( auto& s ) {
		    s.supply.symbol = maximum_supply.symbol;
		    s.max_supply    = maximum_supply;
		    s.issuer        = issuer;
		    });
}

ACTION blacktoken::issue( name to, asset quantity, string memo )
{
	auto sym = quantity.symbol;
	eosio_assert( is_account( to ), "to account does not exist");
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_code_raw = sym.code().raw();
    stats_t statstable( _self, sym_code_raw );
    auto existing = statstable.find( sym_code_raw );
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

	statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
       SEND_INLINE_ACTION( *this, transfer, {st.issuer, "active"_n}, {st.issuer, to, quantity, memo});
	}
}

ACTION blacktoken::transfer( name from, name to, asset quantity, string memo )
{

	eosio_assert( from != to, "cannot transfer to self" );

	require_auth( from );

	eosio_assert( is_account( to ), "to account does not exist");

    auto sym = quantity.symbol.code();
    stats_t statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

	require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	eosio_assert( is_runnable() == true, "contract was frozen");

	// check black list.
	eosio_assert( is_blacklist(from) == false, "not allowed transfer from someone. who was register in blacklist");
	eosio_assert( is_hold_transfer(from, quantity) == false, "partner has locking .... try after expire_date ");

    sub_balance( from, quantity );
	add_balance( to, quantity, from );
}

ACTION blacktoken::retire( asset quantity, string memo ) 
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats_t statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, eosio::same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

ACTION blacktoken::open( name owner, const symbol& symbol, name ram_payer ) 
{
    require_auth( ram_payer );

    auto sym_code_raw = symbol.code().raw();

    stats_t statstable( _self, sym_code_raw );
    const auto st = statstable.get( sym_code_raw, "symbol does not exist" );
    eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

    accounts_t acnts( _self, owner.value );
    const auto it = acnts.find( sym_code_raw );
    if( it == acnts.end() ) {
        acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance = eosio::asset{0, symbol};
        });
    }
}

ACTION blacktoken::close( name owner, const symbol& symbol ) 
{
    require_auth( owner );
    accounts_t acnts( _self, owner.value );
    const auto it = acnts.find( symbol.code().raw() );
    eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
    eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
    acnts.erase( it );
}

void blacktoken::sub_balance( name owner, asset value ) 
{
   accounts_t from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

	from_acnts.modify( from, owner, [&]( auto& a ) {
		a.balance -= value;
	});
}

void blacktoken::add_balance( name owner, asset value, name ram_payer )
{
   accounts_t to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

bool blacktoken::is_permit_transfer( name to, name from )
{
	bool is_permit = false;

	if ( is_blacklist(to) == false && is_blacklist(from) == false)
	{
		is_permit = true;
	}

	return is_permit;
}

ACTION blacktoken::addblklst( name user, const symbol& symbol )
{
	eosio_assert( is_account( user ), "account does not exist");
	eosio_assert(is_blacklist(user) == false, "account_name already exist in blacklist");

	auth_check(symbol);

	auto tc = get_token_config();

	tc.blacklist.push_back(user);

	update_token_config(tc);
}

ACTION blacktoken::rmvblklst( name user, const symbol& symbol )
{
	auth_check(symbol);

	auto tc = get_token_config();

	vector<name>::iterator iter;

	iter = find(tc.blacklist.begin(), tc.blacklist.end(), user);

	eosio_assert( iter != tc.blacklist.end(), "account_name not exist in blacklist");

	tc.blacklist.erase(iter);

	update_token_config(tc);
}

ACTION blacktoken::stop( const symbol& symbol )
{
	auth_check(symbol);

	auto token_config = get_token_config();

	eosio_assert( token_config.is_runnable == true, "contract was frozen");
	eosio::print( " contract going to freeze! \n " );
	token_config.is_runnable = false;

	update_token_config(token_config);
}


ACTION blacktoken::restart( const symbol& symbol )
{
	auth_check(symbol);

	auto token_config = get_token_config();

	eosio_assert( token_config.is_runnable == false, "contract is runing");
	eosio::print( " contract wake up! \n " );
	token_config.is_runnable = true;

	update_token_config(token_config);
}

ACTION blacktoken::addpartner(name user, asset quantity, uint64_t expire_date, string memo )
{
	uint64_t utc = expire_date;
	eosio_assert( is_account( user ), "user account does not exist");

	auto sym = quantity.symbol.code();

	stats_t statstable( _self, sym.raw() );

	const auto& st = statstable.get( sym.raw() );

	require_auth( st.issuer );

	eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
	eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
	eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	eosio_assert( is_runnable() == true, "contract was frozen");

	/// for	test

	if( utc == 0)
	{
		// default locking period is 365 days.
		utc = time_point_sec(now()).utc_seconds + (24 * 3600 * 365);
	}

	/// end

	partners_t reg_acnts( _self, user.value );

	auto partner = reg_acnts.find( quantity.symbol.code().raw() );

	if( partner == reg_acnts.end() ) {
		reg_acnts.emplace( st.issuer, [&]( auto& record ){
				record.balance = quantity;
				record.expire_date = utc;
		});
	} else {
		reg_acnts.modify( partner, eosio::same_payer, [&]( auto& record ) {
				record.balance += quantity;
				record.expire_date = utc;
		});
	}
}

ACTION blacktoken::rmvpartner(name user, const symbol& symbol, string memo )
{
	eosio_assert( is_account( user ), "user account does not exist");

	eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	auth_check(symbol);

	partners_t reg_acnts( _self, user.value );

	auto partner = reg_acnts.find( symbol.code().raw() );

	eosio_assert( partner != reg_acnts.end(), "no reg user name" );

	reg_acnts.erase(partner);
}

bool blacktoken::is_blacklist( name user)
{
	auto tc = get_token_config();

	vector<name>::iterator iter;
	iter = find(tc.blacklist.begin(), tc.blacklist.end(), user);

	return iter == tc.blacklist.end() ? false : true ;

}

bool blacktoken::is_hold_transfer( name user, asset quantity)
{
	bool hold_transfer = true;
	uint64_t current_time;
	partners_t reg_acnts( _self, user.value );

	auto partner = reg_acnts.find( quantity.symbol.code().raw() );

	if (partner == reg_acnts.end())
	{
		hold_transfer = false;
	} 
	else
	{
		auto& partner_info = *partner;

		if (partner_info.expire_date == 0)
		{
			hold_transfer = false;
		} 
		else
		{
			current_time = time_point_sec(now()).utc_seconds;

			if (partner_info.expire_date < current_time)
			{
				hold_transfer = false;
			}
			else if ( partner_info.balance.amount > 0 )
			{
				accounts_t acnts_info( _self, user.value );

				auto account_info = acnts_info.find( quantity.symbol.code().raw());

				if ( account_info == acnts_info.end() )
				{
					hold_transfer = false;
				}
				else
				{
					auto& holder = *account_info;

					if ( holder.balance.amount > partner_info.balance.amount &&
							holder.balance.amount - partner_info.balance.amount >= quantity.amount)
					{
						hold_transfer = false;
					}
					else
					{
						hold_transfer = true;
					}
				}
			}
			else
			{
				hold_transfer = false;

			}
		}
	}

	return hold_transfer;
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
	else 
	{
		available_balance = contract_balance;
	}

	return available_balance;

}

void blacktoken::auth_check( const symbol& symbol )
{
    eosio_assert( symbol.is_valid(), "invalid symbol name" );

    auto sym_code_raw = symbol.code().raw();
	stats_t statstable( _self, sym_code_raw );

    auto existing = statstable.find( sym_code_raw );
    eosio_assert( existing != statstable.end(), "does not exist blacktoken with symbol" );

	const auto& st = *existing;
	require_auth( st.issuer );
}

} // namespace eosblack

EOSIO_DISPATCH( eosblack::blacktoken, (create)(issue)(transfer)(open)(close)(addblklst)(rmvblklst)(stop)(restart)(addpartner)(rmvpartner))
