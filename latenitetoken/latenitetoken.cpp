#include "latenitetoken.hpp"

namespace eosio {

void token::create( name   issuer,
                    asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    // *this = the contract
    // transfer = the name of the action (no quotes)
    // st.issuer = the account that is issuing the tokens 
    // "active"_n = this is the active permission
    // the last line are the vars for a transfer(from, to, asset, memo)
    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void token::sell( name from, name to, asset quantity, string memo ) {
  asset myAsset = asset(10000, symbol("EOS", 4)); 
  print(quantity.amount);
  eosio_assert(quantity >= myAsset, "Must transfer 1 or more EOS");
  eosio_assert(quantity.amount >= 1.0000, "");
  print(quantity >= myAsset);
  //eosio_assert(to == _self, "Sales must include a transfer of eos to the contract");
  SEND_INLINE_ACTION(*this, issue, { {_self, "active"_n} },
    { from, asset(quantity.amount*10000, symbol("YOURTOKENSYMBOL", 4)), memo }
  );
}

void token::transfer( name from, name to, asset quantity, string memo ) {
  eosio_assert( from != to, "cannot transfer to self" );
  require_auth( from );
  eosio_assert( is_account( to ), "to account does not exist");
  auto sym = quantity.symbol.code();
  stats statstable( _self, sym.raw() );
  const auto& st = statstable.get( sym.raw() );
    
  require_recipient( from );
  require_recipient( to );

  eosio_assert( quantity.is_valid(), "invalid quantity" );
  eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
  eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
  eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

  auto payer = has_auth( to ) ? to : from;

  sub_balance( from, quantity );
  print(to);
  add_balance( to, quantity, payer );
}

void token::sub_balance(name owner, asset value) {
  accounts from_acnts(_self, owner.value);

  const auto &from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
  eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

  from_acnts.modify(from, owner, [&](auto &a) {
    a.balance -= value;
  });
}

void token::add_balance( name owner, asset value, name ram_payer ) {
  print("in the add balance method");
  accounts to_acnts( _self, owner.value );
  auto to = to_acnts.find( value.symbol.code().raw() );
  if( to == to_acnts.end() ) {
    print("no account found");
    to_acnts.emplace( ram_payer, [&]( auto& a ){
      a.balance = value;
    });
  } else {
    print("account found adding to balance \n");
    to_acnts.modify( to, same_payer, [&]( auto& a ) {
      print(a.balance);
      a.balance += value;
    });
  }
}

void token::open( name owner, const symbol& symbol, name ram_payer ) {
  require_auth( ram_payer );
  auto sym_code_raw = symbol.code().raw();

  stats statstable( _self, sym_code_raw );
  const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
  eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

  accounts acnts( _self, owner.value );
  auto it = acnts.find( sym_code_raw );
   
  if( it == acnts.end() ) {
    acnts.emplace( ram_payer, [&]( auto& a ){
      print("adding row");
      a.balance = asset{0, symbol};
    });
  }
}

void token::close( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

extern "C" {
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    token contract( name receiver );
    if(code==receiver) {
      // handle all actions where originating with this contract
      if(action==name("issue").value) {
        execute_action(name(receiver), name(code), &token::issue );
      } else if (action==name("open").value) {
        execute_action(name(receiver), name(code), &token::open );
      } else if (action==name("close").value) {
        execute_action(name(receiver), name(code), &token::close );
      } else if (action==name("create").value) {
        execute_action(name(receiver), name(code), &token::create );
      } else if (action==name("transfer").value) {
        execute_action(name(receiver), name(code), &token::transfer );
      } 
    } else if(code==name("eosio.token").value && action== name("transfer").value) {
      // handle eos transfers in only if they came from eosio
      execute_action(name(receiver), name(code), &token::sell );
    }
  }
}

} /// namespace eosio
