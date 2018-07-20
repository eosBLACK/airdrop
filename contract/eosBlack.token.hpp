/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <map>

using namespace std;
using namespace eosio;

namespace eosiosystem {
   class system_contract;
}

namespace eosblack {

	class blacktoken : public contract {

		public:
			blacktoken( account_name self ):contract(self),
										    tokenconfig(_self, _self)
			{ }

			void create( account_name issuer, asset maximum_supply );

			void issue( account_name to, asset quantity, string memo );

			void transfer( account_name from,
						   account_name to,
						   asset        quantity,
						   string       memo );

			void supplement( asset quantity );

			void burn( asset quantity );

			inline asset get_supply( symbol_name sym )const;

			inline asset get_balance( account_name owner, symbol_name sym )const;

			void addblklst( account_name user, asset asset_for_symbol );

			void rmvblklst( account_name user, asset asset_for_symbol );

			inline void stop( asset asset_for_symbol );

			inline void restart( asset asset_for_symbol );

		private:
			// @abi table accounts i64
			struct account {
				asset    balance;

				uint64_t primary_key()const { return balance.symbol.name(); }
			};

			// @abi table stat i64
			struct cur_stats {
				asset          supply;
				asset          max_supply;
				account_name   issuer;

				uint64_t primary_key()const { return supply.symbol.name(); }
			};

			typedef eosio::multi_index<N(accounts), account> accounts;
			typedef eosio::multi_index<N(stat), cur_stats> stats;

			void sub_balance( account_name owner, asset value );
			void add_balance( account_name owner, asset value, account_name ram_payer );

			// @abi table blackcfg i64
			struct tokencfg {
				bool				 is_runnable;
				uint64_t			 create_time;
				uint32_t			 max_period;
				uint32_t			 lock_period;
				uint32_t			 period_uint;
				uint64_t			 lock_balance;
				uint64_t			 balance_uint;
				vector<account_name> blacklist;
			};

			typedef singleton<N(blackcfg), tokencfg> tokencfg_singleton;

			tokencfg_singleton		tokenconfig;

			tokencfg get_token_config();

			void update_token_config( tokencfg &new_token_config );

			void blacktoken_initialize();

			bool is_runnable();

			bool is_valid_account( account_name to );

			inline bool is_permit_transfer( account_name from, account_name to );

			bool is_blacklist( account_name user );

			uint64_t available_balance ( uint64_t contract_balance );

			void auth_check( asset asset_for_symbol );

		public:
			struct transfer_args {
				account_name  from;
				account_name  to;
				asset         quantity;
				string        memo;
			};

		private:
	};

	asset blacktoken::get_supply( symbol_name sym )const
	{
		stats statstable( _self, sym );
		const auto& st = statstable.get( sym );
		return st.supply;
	}

	asset blacktoken::get_balance( account_name owner, symbol_name sym )const
	{
		accounts accountstable( _self, owner );
		const auto& ac = accountstable.get( sym );
		return ac.balance;
	}

} /// namespace eosio
