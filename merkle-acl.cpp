#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/print.hpp>

static constexpr uint64_t token_symbol = S(4, TEST); // precision, symbol

using namespace eosio;
using eosio::asset;
using std::make_pair;


class simpletoken : public eosio::contract {
public:
    simpletoken(account_name self)
        : contract(self)
        , _accounts(_self, _self)
        , _mroots(_self, _self)
    {
    }

    // @abi action
    void merklemint(account_name from, checksum256 acc_name_hash, std::vector<checksum256> merkle_proof)
    {
        //require_auth( from );
        //eosio_assert( merkle_proof.size() <= 255 );

        checksum256 hz_root = traversemerkle(acc_name_hash, merkle_proof);

        auto const & _merkle_acl_root = _mroots.get(N(mroot));

        eosio_assert(hz_root == _merkle_acl_root.mroot, "Account is not ok");

        eosio::print("Account is ok, minted\n");
        issue(from, _merkle_issue_amount);
    }

    // @abi action
    void transfer(account_name from, account_name to, asset quantity)
    {
        require_auth(from);
        eosio_assert(quantity.symbol == token_symbol, "wrong symbol");

        const auto& fromacnt = _accounts.get(from);
        eosio_assert(fromacnt.balance >= quantity, "overdrawn balance");
        _accounts.modify(fromacnt, from, [&](auto& a) { a.balance -= quantity; });

        add_balance(from, to, quantity);
    }

    // @abi action
    void issue(account_name to, asset quantity)
    {
        require_auth(_self);
        eosio_assert(quantity.symbol == token_symbol, "wrong symbol");

        add_balance(_self, to, quantity);
    }

    // @abi action
    void mrklsetprms(checksum256 merkle_root)
    {
        require_auth(_self);
        auto toitr = _mroots.find(N(mroot));
        if (toitr == _mroots.end()) {
            _mroots.emplace(_self, [&](auto& a) {
                a.id = N(mroot);
                a.mroot = merkle_root;
            });
        } else {
            _mroots.modify(toitr, 0, [&](auto& a) {
                a.id = N(mroot);
                a.mroot = merkle_root;
            });
        }
    }

private:
    // [TODO] @abi hz
    asset _merkle_issue_amount = asset(100, token_symbol);

    // @abi table
    struct mroot {
        uint64_t id;
        checksum256 mroot; // hello JS !

        uint64_t primary_key() const { return N(mroot); }
    };

    // @abi table
    struct account {
        account_name owner;
        eosio::asset balance;

        uint64_t primary_key() const { return owner; }
    };

    eosio::multi_index<N(account), account> _accounts;
    eosio::multi_index<N(mroot), mroot> _mroots;

    void add_balance(account_name payer, account_name to, asset q)
    {
        auto toitr = _accounts.find(to);
        if (toitr == _accounts.end()) {
            _accounts.emplace(payer, [&](auto& a) {
                a.owner = to;
                a.balance = q;
            });
        } else {
            _accounts.modify(toitr, 0, [&](auto& a) {
                a.balance += q;
                eosio_assert(a.balance >= q, "overflow detected");
            });
        }
    }

    char* make_canonical_pair(const checksum256 & l, const checksum256 & r)
    {
        static char buf[64];
        memcpy(buf, &l.hash, 32);
        memcpy(buf + 32, &r.hash, 32);
        return buf;
    };

    checksum256 traversemerkle(const checksum256 & leaf, const std::vector<checksum256> & proof)
    {
        checksum256 current = leaf;

        for (auto && el: proof) {
            if (std::less<checksum256>()(current, el)) {
                sha256(make_canonical_pair(current, el), 64, &current);
            } else {
                sha256(make_canonical_pair(el, current), 64, &current);
            }
        }

        return current;
    }
};

EOSIO_ABI(simpletoken, (merklemint)(transfer)(issue)(mrklsetprms))
