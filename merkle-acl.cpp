#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

static constexpr uint64_t token_symbol = S(4, TEST); // precision, symbol

using eosio::asset;

class simpletoken : public eosio::contract {
   public:
      simpletoken( account_name self )
      :contract(self),_accounts( _self, _self){}

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

   private:
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
};

EOSIO_ABI( simpletoken, (transfer)(issue) )
