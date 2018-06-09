#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/types.hpp>
#include <eosiolib/print.hpp>


static constexpr uint64_t token_symbol = S(4, TEST); // precision, symbol

using namespace eosio;
using eosio::asset;


class simpletoken : public eosio::contract {
   public:
      simpletoken( account_name self )
      :contract(self),_accounts( _self, _self){}

      // @abi action
      void merklemint(account_name from, checksum256 acc_name_hash, std::vector<checksum256> merkle_proof) {
      	require_auth( from );
			char *jopajopa="aaaaw";
			// [ASK] check if not empty
			// eosio_assert( merkle_proof.size() <= 255 );
			uint8_t proof_size = merkle_proof.size();
			checksum256 hz_root = traverse_merkle(acc_name_hash, merkle_proof);
			if (memcmp(&hz_root, &_merkle_acl_root, sizeof(hz_root))) {
				print(666);			
			}
			print(222);
  		}

      // @abi action
      void transfer( account_name from, account_name to, asset quantity ) {
         require_auth( from );
         eosio_assert( quantity.symbol == token_symbol, "wrong symbol" );

         const auto& fromacnt = _accounts.get( from );
         eosio_assert( fromacnt.balance >= quantity, "overdrawn balance" );
         _accounts.modify( fromacnt, from, [&]( auto& a ){ a.balance -= quantity; } );

         add_balance( from, to, quantity );
      }

      // @abi action
      void issue( account_name to, asset quantity ) {
         require_auth( _self );
         eosio_assert( quantity.symbol == token_symbol, "wrong symbol" );

         add_balance( _self, to, quantity );
      }

      // @abi action
      void set_merkle_root_and_issue_amount(checksum256 merkle_root, asset amount_to_issue) {
         require_auth( _self );
         eosio_assert( amount_to_issue.symbol == token_symbol, "wrong symbol" );
			_merkle_acl_root = merkle_root;
			_merkle_issue_amount = amount_to_issue;
      }

   private:
		// [TODO] @abi hz
	  	checksum256 _merkle_acl_root; // hello JS !
      asset _merkle_issue_amount = asset(100, token_symbol);

      // @abi table
      struct account {
         account_name owner;
         eosio::asset balance;

         uint64_t primary_key()const { return owner; }
      };

      eosio::multi_index<N(account), account> _accounts;

      void add_balance( account_name payer, account_name to, asset q ) {
         auto toitr = _accounts.find( to );
         if( toitr == _accounts.end() ) {
           _accounts.emplace( payer, [&]( auto& a ) {
              a.owner = to;
              a.balance = q;
           });
         } else {
           _accounts.modify( toitr, 0, [&]( auto& a ) {
              a.balance += q;
              eosio_assert( a.balance >= q, "overflow detected" );
           });
         }
      }

	 	checksum256 traverse_merkle(checksum256 leaf, vector<checksum256> ids) {                                                                                                                 
		 	eosio_assert(0 != ids.size(), "aaaaaaaaaaaaaaaaA");

		 	checksum256 cur = leaf;
		 	char len = sizeof(leaf);
		 	void *buf = malloc(2 * len);
																																			
		 	for (int i = 0; i < ids.size(); i++) {
			 	if (memcmp(&ids[i], &cur, sizeof(ids[i])) < 0) {
				 	memcpy(&buf, &ids[i], len);
				 	memcpy(&buf + len, &leaf, len);
			 	} else {
				 	memcpy(&buf, &leaf, len);
				 	memcpy(&buf + len, &ids[i], len);
			 	}
			 	sha256((char *)buf, len * 2, &cur);
		 	}
			return cur;                                                                                                                                        
		}                       
};

EOSIO_ABI( simpletoken, (merklemint)(transfer)(issue) )
