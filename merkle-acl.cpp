#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/types.hpp>
#include <eosiolib/print.hpp>


static constexpr uint64_t token_symbol = S(4, TEST); // precision, symbol

using namespace eosio;
using eosio::asset;
using std::make_pair;

class simpletoken : public eosio::contract {
   public:
      simpletoken( account_name self )
      :contract(self),_accounts( _self, _self){}

      // @abi action
      void merklemint(account_name from, checksum256 acc_name_hash, std::vector<checksum256> merkle_proof) {
      	//require_auth( from );
			//eosio_assert( merkle_proof.size() <= 255 );
			checksum256 hz_root = traversemerkle(acc_name_hash, merkle_proof);
			if (memcmp(&hz_root, &_merkle_acl_root, sizeof(checksum256)) == 0) {
				issue(from, _merkle_issue_amount);
				eosio::print("Account is ok, minted");
				return;
			}
			eosio::print("Account is not ok");
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
		bool is_canonical_left(const checksum256& val) {
			char check = val.hash[7];
			if ((check & 0x80) == 0) {
				return true;
			}
			return false;                                                                                                                            
		}                                                                                                                                                             
																																																						  
		checksum256 make_canonical_left(const checksum256& val) {                                                                                                     
			return val;
			checksum256 canonical_l = val;
			for(char i=0; i < 7; i++) {
				canonical_l.hash[i] &= 0xFF;
			} 
			canonical_l.hash[7] &= 0x7F;                                                                                                                            
			return canonical_l;                                                                                                                                        
		}                                                                                                                                                             
																																																						  
		checksum256 make_canonical_right(const checksum256& val) {                                                                                                    
			checksum256 canonical_r = val;                                                                                                                             
			for(char i=0; i < 7; i++) {
				canonical_r.hash[i] |= 0x00;
			} 
			canonical_r.hash[7] |= 0x80;;                                                                                                                            
			return canonical_r;                                                                                                                                        
		}     
																																																								  
   	char* make_canonical_pair(const checksum256& l, const checksum256& r) {
		 	char len = sizeof(checksum256);
			static char buf[sizeof(checksum256) * 2];
			memcpy(&buf, &l, len);
			memcpy(&buf + len, &r, len);
			return buf;
   	};
          
	 	checksum256 traversemerkle(checksum256 leaf, std::vector<checksum256> ids) {
			checksum256 current = leaf;
			unsigned char len = sizeof(checksum256);
 
			for (checksum256& p: ids ) {
      		if (is_canonical_left(p)) {
         		sha256((char *)make_canonical_pair(p, current), len * 2, &current);
					
      		} else {                                                                                                                                                
         		sha256((char *)make_canonical_pair(current, p), len * 2, &current);
				}
      	}
			return current;                                                                                                                                                       
   	}                                                                                                                                                          
};

EOSIO_ABI( simpletoken, (merklemint)(transfer)(issue) )
