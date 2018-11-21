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

namespace eosblack {

	CONTRACT blacktoken : public contract {

		public:
			blacktoken( name receiver, name code, datastream<const char*> ds ) :
            	contract(receiver, code, ds),
				tokenconfig(receiver, receiver.value)
			{ }

			ACTION create( name issuer, asset maximum_supply );
			ACTION issue( name to, asset quantity, string memo );
			ACTION transfer( name from, name to, asset quantity, string memo );
			ACTION retire( asset quantity, string memo );
			ACTION open( name owner, const symbol& symbol, name ram_payer );
			ACTION close( name owner, const symbol& symbol );

			ACTION addblklst( name user, const symbol& symbol );
			ACTION rmvblklst( name user, const symbol& symbol );
			ACTION stop( const symbol& symbol );
			ACTION restart( const symbol& symbol );

			ACTION addpartner( name account, asset quantity, uint64_t expire_date, string memo );
			ACTION rmvpartner( name account, const symbol& symbol, string memo );

			static asset get_supply( name token_contract_account, symbol_code sym_code )
			{
				stats_t statstable( token_contract_account, sym_code.raw() );
				const auto& st = statstable.get( sym_code.raw() );
				return st.supply;
			}

			static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
			{
				accounts_t accountstable( token_contract_account, owner.value );
				const auto& ac = accountstable.get( sym_code.raw() );
				return ac.balance;
			}

		private:
			TABLE account {
				asset    balance;

				uint64_t primary_key()const { return balance.symbol.code().raw(); }
			};

			TABLE currency_stats {
				asset          supply;
				asset          max_supply;
				name		   issuer;

				uint64_t primary_key()const { return supply.symbol.code().raw(); }
			};

			typedef eosio::multi_index< "accounts"_n, account > accounts_t;
			typedef eosio::multi_index< "stat"_n, currency_stats > stats_t;

			void sub_balance( name owner, asset value );
			void add_balance( name owner, asset value, name ram_payer );

			TABLE partner {
				asset			balance;
				uint64_t		expire_date;

				uint64_t primary_key()const { return balance.symbol.code().raw(); }
			};

			typedef eosio::multi_index< "partners"_n, partner > partners_t;

			//TABLE tokencfg {
			struct [[eosio::table("blackcfg")]]	tokencfg {
				bool				 is_runnable;
				uint64_t			 create_time;
				uint32_t			 max_period;
				uint32_t			 lock_period;
				uint32_t			 period_uint;
				uint64_t			 lock_balance;
				uint64_t			 balance_uint;
				vector<name> 		 blacklist;
			};

			typedef singleton<"blackcfg"_n, tokencfg> tokencfg_singleton;

			tokencfg_singleton		tokenconfig;

			tokencfg get_token_config();

			void update_token_config( tokencfg &new_token_config );

			void blacktoken_initialize();

			bool is_runnable();

			bool is_valid_account( name to );

			inline bool is_permit_transfer( name from, name to );

			bool is_blacklist( name user );

			bool is_hold_transfer( name user, asset quantity);

			uint64_t available_balance ( uint64_t contract_balance );

			void auth_check( const symbol& symbol );

		public:
			struct transfer_args {
				name  		from;
				name  		to;
				asset       quantity;
				string      memo;
			};

		private:
	};


} /// namespace eosio
